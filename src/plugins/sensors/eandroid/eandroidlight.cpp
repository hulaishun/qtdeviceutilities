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
#include <eandroidlight.h>

EAndroidLight::EAndroidLight(int type, QSensor *sensor)
    : EAndroidBaseSensor(type, sensor)
{
    setReading<QLightReading>(&m_reading);
}

EAndroidLight::~EAndroidLight()
{
}

void EAndroidLight::processEvent(sensors_event_t &event)
{
    m_reading.setTimestamp(event.timestamp / 1000);
    m_reading.setLux(event.light);
    newReadingAvailable();
}
