/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Device Utilities module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QNETWORKSETTINGSINTERFACEPRIVATE_H
#define QNETWORKSETTINGSINTERFACEPRIVATE_H

#include <QObject>
#include "qnetworksettings.h"
#include "qnetworksettingsinterface.h"
#include "qnetworksettingsmanager_p.h"

class QNetworkSettingsInterfacePrivate : public QObject
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(QNetworkSettingsInterface)
public:
    explicit QNetworkSettingsInterfacePrivate(QNetworkSettingsInterface* parent);
    void initialize(const QString& path, const QVariantMap& properties);
    void setPowered(const bool power);
    void setState(QNetworkSettingsState::States aState);
    void setManager(QNetworkSettingsManagerPrivate *manager);
    void scan();

Q_SIGNALS:

protected:
    QString m_name;
    QNetworkSettingsType m_type;
    QNetworkSettingsState m_state;
    bool m_powered;
    QNetworkSettingsManagerPrivate *m_manager; //not owned
    QNetworkSettingsInterface *q_ptr;
};

class WpaSupplicantSettingsInterface : public QNetworkSettingsInterface
{
    Q_OBJECT
public:
    WpaSupplicantSettingsInterface(QNetworkSettingsManagerPrivate* manager)
        :QNetworkSettingsInterface(manager)
    {
        Q_D(QNetworkSettingsInterface);
        d->setManager(manager);
    }

    void setState(QNetworkSettingsState::States aState) {
        Q_D(QNetworkSettingsInterface);
        d->setState(aState);
    }

    virtual ~WpaSupplicantSettingsInterface() {
    }
};

#endif // QNETWORKSETTINGSINTERFACEPRIVATE_H
