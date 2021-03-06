/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use the contact form at
** http://qt.digia.com/
**
** This file is part of Qt Enterprise Embedded.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** the contact form at http://qt.digia.com/
**
****************************************************************************/

#include "elevator.h"

#include <QFile>
#include <QDebug>
#include <QProcess>

QStringList elevate()
{
    QStringList rc;

    int trySudo = QProcess::execute("sudo", QStringList() << "-n" << "echo" << "foo");
    if (trySudo == 0) {
        qDebug() << "Sudo seems to work";
        rc << "sudo" << "--";
        return rc;
    }

    if (QFile::exists("/usr/bin/pkexec"))
        rc << "/usr/bin/pkexec"; // @ARGS

    return rc;
}

