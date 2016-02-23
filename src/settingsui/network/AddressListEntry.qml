/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Device Utilities module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
import QtQuick 2.6
import QtQuick.Layouts 1.3
import Qt.labs.controls 1.0
import Qt.labs.controls.material 1.0
import Qt.labs.controls.universal 1.0

RowLayout {
    id: root
    spacing: 10
    property alias model: repeater.model
    property bool modified: false
    property alias title: label.text

    Label {
        id: label
        Layout.preferredWidth: content.titleWidth
        horizontalAlignment: Text.AlignRight
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: 10
    }
    ColumnLayout {
        spacing: 10
        Layout.fillWidth: true

        Repeater {
            id: repeater
            visible: count > 0
            RowLayout {
                spacing: 10
                TextField {
                    text: edit
                    Layout.fillWidth: true
                    onEditingFinished: {
                        root.modified = true;
                        edit = text;
                    }
                }
                Button {
                    Layout.preferredWidth: height
                    text: "-"
                    visible: repeater.count > 0
                    onClicked: {
                        root.modified = true;
                        root.model.remove(index);
                    }
                }
                Button {
                    Layout.preferredWidth: height
                    visible: index === repeater.count - 1
                    text: "+"
                    onClicked: root.model.append("")

                    Layout.alignment: Qt.AlignRight
                }
            }
        }
        RowLayout {
            visible: repeater.count === 0
            spacing: 10
            TextField {
                id: nameServerEntryItem
                Layout.fillWidth: true
                text: ""
                onAccepted: root.model.append(text)
            }
            Button {
                Layout.preferredWidth: height
                text: "+"
                onClicked: nameServerEntryItem.accepted()
            }
        }
    }
}
