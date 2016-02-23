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
import com.theqtcompany.settings.locale 1.0
import "../common"

Item {
    id: root
    property string title: qsTr("Select language")

    Component.onCompleted: country.text = LocaleFilter.filter

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: 20
        spacing: 10
        RowLayout {
            spacing: 10

            Label {
                text: qsTr("Search region: ")
                Layout.alignment: Qt.AlignVCenter
            }
            TextField {
                id: country
                text: ""
                onTextChanged: LocaleFilter.filter = country.text
                Layout.alignment: Qt.AlignVCenter
            }
        }
        CustomTableView {
            headerTexts: [qsTr("Language"), qsTr("Country")]
            roleNames: ["language", "country"]
            model: LocaleFilter
            onClicked: {
                var val = model.itemFromRow(index);
                if (val !== "") {
                    LocaleManager.locale = val;
                    stackView.pop();
                }
            }
        }
    }
}
