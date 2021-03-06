/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use the contact form at
** http://www.qt.io
**
** This file is part of Qt Enterprise Embedded.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** the contact form at http://www.qt.io
**
****************************************************************************/
#include <QtCore>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

#include <unistd.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <netutils/dhcp.h>
#include <netutils/ifc.h>

// Code values come from android/system/netd/ResponseCode.h
static const int InterfaceChange = 600;

static const char UNIQUE_HOSTNAME[] = "net.hostname";

static bool QT_CONNECTIVITY_DEBUG = !qgetenv("QT_CONNECTIVITY_DEBUG").isEmpty();
static bool QT_USE_EXPIRED_LEASE = !qgetenv("QT_USE_EXPIRED_LEASE").isEmpty();

// sanity check a renewal time, lower value than
// this might indicate a badly configured DHCP server
static int MIN_RENEWAL_TIME_SECS = 300; // 5 min

#define ETH_INTERFACE_HW "eth0"
#define ETH_INTERFACE_EMULATOR "eth1"

#if Q_ANDROID_VERSION_MAJOR == 4 && Q_ANDROID_VERSION_MINOR < 3
// this function is defined in android/system/core/libnetutils/dhcp_utils.c
extern "C" {
int dhcp_do_request_renew(const char *ifname,
                          char *ipaddr,
                          char *gateway,
                          uint32_t *prefixLength,
                          char *dns1,
                          char *dns2,
                          char *server,
                          uint32_t *lease,
                          char *vendorInfo);
}
#endif

static int q_dhcp_do_request(bool renew,
                             const char *ifname,
                             char *ipaddr,
                             char *gateway,
                             uint32_t *prefixLength,
                             char *dns1,
                             char *dns2,
                             char *server,
                             uint32_t *lease,
                             char *vendorInfo,
                             char *domain)
{
#if Q_ANDROID_VERSION_MAJOR == 4 && Q_ANDROID_VERSION_MINOR < 3
    if (!renew)
        return dhcp_do_request(ifname, ipaddr, gateway, prefixLength, dns1, dns2, server, lease, vendorInfo);
    return dhcp_do_request_renew(ifname, ipaddr, gateway, prefixLength, dns1, dns2, server, lease, vendorInfo);
#else
    char *dns[3] = {dns1, dns2, 0};
    char mtu[PROPERTY_VALUE_MAX];
#if Q_ANDROID_VERSION_MAJOR == 4 && Q_ANDROID_VERSION_MINOR < 4
    if (!renew)
        return dhcp_do_request(ifname, ipaddr, gateway, prefixLength, dns, server, lease, vendorInfo);
    return dhcp_do_request_renew(ifname, ipaddr, gateway, prefixLength, dns, server, lease, vendorInfo);
#else
    if (!renew)
        return dhcp_do_request(ifname, ipaddr, gateway, prefixLength, dns, server, lease, vendorInfo, domain, mtu);
    return dhcp_do_request_renew(ifname, ipaddr, gateway, prefixLength, dns, server, lease, vendorInfo, domain, mtu);
#endif
#endif
}


class LeaseTimer;
class QConnectivityDaemon : public QObject
{
    Q_OBJECT
public:
    QConnectivityDaemon();

protected:
    void setHostnamePropery(const char *interface) const;
    void sendCommand(const char *command) const;
    void handleInterfaceChange(const QList<QByteArray> &message);
    bool startDhcp(bool renew, const char *interface);
    void stopDhcp(const char *interface);
    bool ethernetSupported() const;
    bool isEmulator() const;

protected slots:
    void initNetdConnection();
    void handleNetdEvent();
    void handleRequest();
    void handleNewConnection();
    void sendReply(QLocalSocket *requester, const QByteArray &reply) const;
    void updateLease();
    void handleError(QLocalSocket::LocalSocketError /*socketError*/) const;

private:
    friend class LeaseTimer;
    QLocalSocket *m_netdSocket;
    // currently used to listen for requests from Qt Wifi library,
    // in future can be used also for Bluetooth and etc.
    QLocalServer *m_serverSocket;
    bool m_linkUp;
    LeaseTimer *m_leaseTimer;
    bool m_isEmulator;
    QByteArray m_ethInterface;
    // android initializes services in a separate threads, therefore it is
    // not guaranteed that the netd socket will be ready at the time when qconnectivity
    // is starting up - we try to reconnect again after 2 second intervals. This
    // variable holds the maximum attempt count (chosen arbitrarily).
    int m_attemptCount;
};

class LeaseTimer : public QTimer
{
    Q_OBJECT
public:
    LeaseTimer(QConnectivityDaemon *daemon) : m_daemon(daemon) {}

    void setInterface(const QByteArray &interface)
    {
        if (m_ifc.isEmpty()) {
            m_ifc = interface;
        } else {
            // for example when user switches from eth0 to wlan0, we
            // stop DHCP on the previous interface
            if (m_ifc != interface) {
                m_daemon->stopDhcp(m_ifc.constData());
                m_ifc = interface;
            }
        }
    }

    QByteArray interface() const { return m_ifc; }

private:
    QConnectivityDaemon *m_daemon;
    QByteArray m_ifc;
};

QConnectivityDaemon::QConnectivityDaemon()
    : m_netdSocket(0),
      m_serverSocket(0),
      m_linkUp(false),
      m_leaseTimer(0),
      m_isEmulator(isEmulator()),
      m_attemptCount(50)
{
    qDebug() << "starting QConnectivityDaemon...";
    if (!m_isEmulator) {
        m_ethInterface = ETH_INTERFACE_HW;
        m_leaseTimer = new LeaseTimer(this);
        m_leaseTimer->setSingleShot(true);
        connect(m_leaseTimer, SIGNAL(timeout()), this, SLOT(updateLease()));

        int serverFd = socket_local_server("qconnectivity", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
        if (serverFd != -1) {
            m_serverSocket = new QLocalServer(this);
            if (m_serverSocket->listen(serverFd))
                connect(m_serverSocket, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
            else
                qWarning() << "QConnectivityDaemon: not able to listen on the server socket...";
        } else {
            qWarning() << "QConnectivityDaemon: failed to open qconnectivity server socket";
        }
    } else {
        m_ethInterface = ETH_INTERFACE_EMULATOR;
    }
    initNetdConnection();
}

bool QConnectivityDaemon::isEmulator() const
{
    bool isEmulator = false;
    QFile conf("/system/bin/appcontroller.conf");
    if (conf.open(QIODevice::ReadOnly)) {
        QByteArray content = conf.readAll();
        isEmulator = content.contains("platform=emulator");
        conf.close();
    } else {
        qWarning() << "Failed to read appcontroller.conf";
    }
    return isEmulator;
}

void QConnectivityDaemon::initNetdConnection()
{
    int netdFd = socket_local_client("netd", ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
    if (netdFd != -1) {
        qDebug() << "QConnectivityDaemon: connected to netd socket";
        m_netdSocket = new QLocalSocket(this);
        m_netdSocket->setSocketDescriptor(netdFd);
        connect(m_netdSocket, SIGNAL(readyRead()), this, SLOT(handleNetdEvent()));
        connect(m_netdSocket, SIGNAL(error(QLocalSocket::LocalSocketError)),
                this, SLOT(handleError(QLocalSocket::LocalSocketError)));
    } else {
        qWarning() << "QConnectivityDaemon: failed to connect to netd socket, reattempting...";
        if (--m_attemptCount != 0)
            QTimer::singleShot(200, this, SLOT(initNetdConnection()));
        return;
    }
    if (ethernetSupported()) {
        // down-up sequence generates "linkstate" events, which we can use to setup
        // our daemon on initial startup (device boot) or on daemon restarts
        sendCommand(QByteArray("0 interface setcfg ").append(m_ethInterface).append(" down").constData());
        sendCommand(QByteArray("0 interface setcfg ").append(m_ethInterface).append(" up").constData());
    }
    // disable firewall - this setting seems to be enabled only when using "Always-on VPN"
    // mode on Android phones, see setLockdownTracker() in ConnectivityService.java
    sendCommand("0 firewall disable");
}

void QConnectivityDaemon::setHostnamePropery(const char *interface) const
{
    // Setup our unique device name (used as a host name argument for dhcpcd call in
    // dhcp_do_request). On Android device name is set in ConnectivityService.java and
    // the id is generated with the help of SecureRandom.java class. We will use Mac
    // address as a unique hostname.
    char prop_value[PROPERTY_VALUE_MAX];
    property_get(UNIQUE_HOSTNAME, prop_value, NULL);
    if ((prop_value[0] == '\0')) {
        char hwaddr[6];
        memset(hwaddr, 0, sizeof(hwaddr));
        ifc_init();
        if (ifc_get_hwaddr(interface, (void *)hwaddr) == 0) {
            QByteArray macAddress(hwaddr, sizeof(hwaddr));
            property_set(UNIQUE_HOSTNAME, macAddress.toHex().prepend("b2qt-").constData());
        } else {
            qWarning() << "QConnectivityDaemon: failed to get MAC address";
        }
        ifc_close();
    }
}

void QConnectivityDaemon::sendCommand(const char *command) const
{
    if (!m_netdSocket) {
        qDebug() << "QConnectivityDaemon: netd socket is not ready!";
        return;
    }
    qDebug() << "QConnectivityDaemon: sending command - " << command;
    // netd expects "\0" terminated commands...
    m_netdSocket->write(command, qstrlen(command) + 1);
    m_netdSocket->flush();
}

void QConnectivityDaemon::handleInterfaceChange(const QList<QByteArray> &message)
{
    // Format: "Code Iface linkstate <name> <up/down>"
    if (message.size() < 5)
        return;

    if (message.at(2) == "linkstate" && message.at(3) == m_ethInterface) {
        if (message.at(4) == "up") {
            // ethernet cable has been plugged in
            if (!m_linkUp) {
                m_linkUp = true;
                startDhcp(false, m_ethInterface);
            }
        } else {
            // .. plugged out
            if (m_linkUp) {
                m_linkUp = false;
                stopDhcp(m_ethInterface);
            }
        }
    }
}

bool QConnectivityDaemon::startDhcp(bool renew, const char *interface)
{
    qDebug() << "QConnectivityDaemon: startDhcp [ renew" << renew << "] "
             << "interface: " << interface;
    setHostnamePropery(interface);

    int result = 0;
    char  ipaddr[PROPERTY_VALUE_MAX];
    quint32 prefixLength = 0;
    char gateway[PROPERTY_VALUE_MAX];
    char    dns1[PROPERTY_VALUE_MAX];
    char    dns2[PROPERTY_VALUE_MAX];
    char  server[PROPERTY_VALUE_MAX];
    quint32 lease = 0;
    char vendorInfo[PROPERTY_VALUE_MAX];
    char domain[PROPERTY_VALUE_MAX] = {0};

    if (renew) {
        result = q_dhcp_do_request(true, interface, ipaddr, gateway, &prefixLength,
                                   dns1, dns2, server, &lease, vendorInfo, domain);
    } else {
        // stop any existing DHCP daemon before starting new
        dhcp_stop(interface);
        // this uses "ctl.start.*" mechanism to start "dhcpcd" daemon as defined by
        // the device init.rc. Android starts dhcpcd with argument -B which means that
        // we are responsible for renewing a lease before it expires
        ifc_clear_addresses(interface);
        result = q_dhcp_do_request(false, interface, ipaddr, gateway, &prefixLength,
                                   dns1, dns2, server, &lease, vendorInfo, domain);
    }

    bool success = (result == 0) ? true : false;
    if (success) {
        qDebug() << "\nipaddr: " << ipaddr << "\nprefixLength: " << prefixLength
                 << "\ngateway: " << gateway << "\ndns1: " << dns1 << "\ndns2: " << dns2;

        if (!renew) {
            in_addr _ipaddr, _gateway, _dns1, _dns2;
            inet_aton(ipaddr, &_ipaddr);
            inet_aton(gateway, &_gateway);
            inet_aton(dns1, &_dns1);
            inet_aton(dns2, &_dns2);

            ifc_configure(interface, _ipaddr.s_addr, prefixLength,
                          _gateway.s_addr, _dns1.s_addr, _dns2.s_addr);

            // set DNS servers for interface - see NetworkManagementService.java
            if (domain[0]) {
                QByteArray dnsForInterface("0 resolver setifdns ");
                dnsForInterface.append(interface).append(" ").append(domain).append(" ");
                dnsForInterface.append(dns1).append(" ").append(dns2);
                sendCommand(dnsForInterface.constData());
            }
            // set default interface for DNS - see NetworkManagementService.java
            sendCommand(QByteArray("0 resolver setdefaultif ").append(interface).constData());

            property_set("net.dns1", dns1);
            property_set("net.dns2", dns2);
        }

        if (!m_isEmulator && lease >= 0) {
            if (lease < MIN_RENEWAL_TIME_SECS) {
                qWarning() << "QConnectivityDaemon: DHCP server proposes lease time " << lease
                           << "seconds. We will use" << MIN_RENEWAL_TIME_SECS << " seconds instead.";
                lease = MIN_RENEWAL_TIME_SECS;
            }
            // update lease when 48% of lease time has elapsed
            if (m_leaseTimer->isActive())
                m_leaseTimer->stop();
            m_leaseTimer->setInterface(interface);
            m_leaseTimer->start(lease * 480);
        }
    } else {
        qWarning("QConnectivityDaemon: DHCP request failed - %s", dhcp_get_errmsg());
        if (renew) {
            // If it fails to renew a lease (faulty server, proxy?) we re-connect.
            // Some users might prefer to use expired lease over having interrupt
            // in network connection
            if (QT_USE_EXPIRED_LEASE)
                return true;
            qDebug() << "QConnectivityDaemon: attempting to re-connect...";
            stopDhcp(interface);
            startDhcp(false, interface);
        }
    }
    return success;
}

void QConnectivityDaemon::stopDhcp(const char *interface)
{
    qDebug() << "QConnectivityDaemon: stopDhcp: " << interface;
    ifc_clear_addresses(interface);
    dhcp_stop(interface);
    if (!m_isEmulator && m_leaseTimer->isActive())
        m_leaseTimer->stop();
}

bool QConnectivityDaemon::ethernetSupported() const
{
    // standard linux kernel path
    return QDir().exists(QString("/sys/class/net/").append(m_ethInterface));
}

void QConnectivityDaemon::handleNetdEvent()
{
    QByteArray data = m_netdSocket->readAll();
    if (QT_CONNECTIVITY_DEBUG)
        qDebug() << "QConnectivityDaemon: netd event: " << data;
    if (data.endsWith('\0'))
        data.chop(1);

    QList<QByteArray> message = data.split(' ');
    int code = message.at(0).toInt();
    switch (code) {
    case InterfaceChange:
        handleInterfaceChange(message);
        break;
    default:
        break;
    }
}

void QConnectivityDaemon::handleRequest()
{
    // Format: "interface <connect/disconnect>"
    QLocalSocket *requester = qobject_cast<QLocalSocket *>(QObject::sender());
    if (requester->canReadLine()) {
        QByteArray request = requester->readLine(requester->bytesAvailable());

        qDebug() << "QConnectivityDaemon: received a request: " << request;
        QList<QByteArray> cmd = request.split(' ');
        if (cmd.size() < 2)
            return;

        QByteArray interface = cmd.at(0);
        if (cmd.at(1) == "connect") {
            QByteArray reply;
            if (startDhcp(false, interface.constData()))
                reply = "success";
            else
                reply = "failed";
            sendReply(requester, reply);
        } else {
            stopDhcp(interface.constData());
        }
    }
}

void QConnectivityDaemon::handleNewConnection()
{
    QLocalSocket *requester = m_serverSocket->nextPendingConnection();
    connect(requester, SIGNAL(readyRead()), this, SLOT(handleRequest()));
    connect(requester, SIGNAL(disconnected()), requester, SLOT(deleteLater()));
}

void QConnectivityDaemon::sendReply(QLocalSocket *requester, const QByteArray &reply) const
{
    QByteArray r = reply;
    r.append("\n");
    requester->write(r.constData(), r.length());
    requester->flush();
    requester->disconnectFromServer();
}

void QConnectivityDaemon::updateLease()
{
    qDebug() << "QConnectivityDaemon: updating lease";
    startDhcp(true, m_leaseTimer->interface().constData());
}

void QConnectivityDaemon::handleError(QLocalSocket::LocalSocketError /*socketError*/) const
{
    qWarning() << "QConnectivityDaemon: QLocalSocket::LocalSocketError";
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QConnectivityDaemon connectivityDaemon;

    return a.exec();
}

#include "main.moc"
