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
#ifndef LOCAL_WIFI_H
#define LOCAL_WIFI_H

#include <QtCore/QLoggingCategory>

#include <string.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(B2QT_WIFI)

// This API mirrors Android's Wi-Fi libraries interface [1] and parts of implementation.
// [1] http://androidxref.com/4.4.2_r2/xref/hardware/libhardware_legacy/include/hardware_legacy/wifi.h

int q_wifi_command(const char *ifname, const char *command, char *reply, size_t *reply_len);
int q_wifi_wait_for_event(const char *ifname, char *buf, size_t buflen);
int q_wifi_connect_to_supplicant(const char *ifname);
void q_wifi_close_supplicant_connection(const char *ifname);
int q_wifi_start_supplicant();
int q_wifi_stop_supplicant();

QT_END_NAMESPACE

#endif // LOCAL_WIFI_H
