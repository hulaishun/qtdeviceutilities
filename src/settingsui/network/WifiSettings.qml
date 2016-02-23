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
import com.theqtcompany.settings.network 1.0

Item {
    id: root
    anchors.fill: parent
    Component.onCompleted: NetworkSettingsManager.services.type = NetworkSettingsType.Wifi;

    GroupBox {
        id: content
        title: qsTr("Wireless Settings")
        anchors.fill: parent

        ColumnLayout {
            spacing: 20
            width: parent.width

            RowLayout {
                spacing: 10
                id: enableSwitch
                width: parent.width

                Label {
                    Layout.preferredWidth: root.width * 0.382
                    Layout.alignment: Qt.AlignVCenter
                    horizontalAlignment: Text.AlignRight
                    text: selectedInterface.powered ? qsTr("Wi-Fi OFF") : qsTr("Wi-Fi ON")
                }
                Switch {
                    checked: selectedInterface.powered
                    onCheckedChanged: selectedInterface.powered = checked
                }
            }
            RowLayout {
                spacing: 10
                width: parent.width

                Label {
                    Layout.preferredWidth: root.width * 0.382
                    text: qsTr("Current network")
                    horizontalAlignment: Text.AlignRight
                    Layout.alignment: Qt.AlignVCenter
                }
                ComboBoxEntry {
                    id: networkSelection
                    model: NetworkSettingsManager.services
                    textRole: "name"
                    Layout.fillWidth: true
                    onCurrentIndexChanged: if (currentIndex >= 0) model.itemFromRow(currentIndex).connectService();

                    delegate: WifiSelectorDelegate {
                        onConnectChanged: if (connect) networkSelection.currentIndex = networkSelection.find(modelData.name)
                    }
                }
            }
            GroupBox {
                id: connectView
                title: qsTr("Enter a password")
                visible: false
                Layout.fillWidth: true
                ColumnLayout {
                    width: parent.width

                    RowLayout {
                        id: errorView
                        visible: text.text !== ""
                        spacing: 10
                        property alias text: text.text

                        Image {
                            source: "Alert_yellow_1x.png"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Text {
                            id: text
                            color: "#face20"
                            text: ""
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                    RowLayout {
                        spacing: 10
                        visible: false
                        width: parent.width

                        Label {
                            text: qsTr("User name:")
                            horizontalAlignment: Text.AlignRight
                            Layout.preferredWidth: root.width * 0.382
                            Layout.alignment: Qt.AlignVCenter
                        }
                        TextField {
                            text: ""
                            inputMethodHints: Qt.ImhNoPredictiveText
                            Layout.alignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }
                    }
                    RowLayout {
                        spacing: 10
                        width: parent.width

                        Label {
                            text: qsTr("Password:")
                            horizontalAlignment: Text.AlignRight
                            Layout.preferredWidth: root.width * 0.382
                            Layout.alignment: Qt.AlignVCenter
                        }
                        TextField {
                            id: password
                            text: ""
                            echoMode: TextInput.Password
                            inputMethodHints: Qt.ImhNoPredictiveText
                            Layout.alignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }
                    }
                    RowLayout {
                        spacing: 10

                        Button {
                            text: qsTr("Connect")
                            onClicked: {
                                connectView.visible = false
                                NetworkSettingsUserAgent.setUserCredentials("", password.text)
                            }
                        }
                        Button {
                            text: qsTr("Cancel")
                            onClicked:connectView.visible = false
                        }
                    }
                }
            }
            Button {
                id: disconnect
                text: qsTr("Disconnect")
                visible: selectedInterface.state === NetworkSettingsState.Online ||
                         selectedInterface.state === NetworkSettingsState.Ready
                onClicked: {
                    console.log("disconnect");
                    NetworkSettingsManager.services.itemFromRow(networkSelection.selectedIndex).disconnectService();
                    networkSelection.selectedIndex = -1;
                }
            }
        }
        Connections {
            target: NetworkSettingsUserAgent
            onShowUserCredentialsInput : {
                connectView.visible = true
            }
            onError: {
                errorView.visible = true
                connectView.visible = true
            }
        }
    }
}
