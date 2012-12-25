/*
    This file is part of Korva.

    Copyright (C) 2012 Openismus GmbH.
    Author: Jens Georg <jensg@openismus.com>

    Korva is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Korva is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Korva.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 1.1
import com.nokia.meego 1.0
import org.jensge.PushUp 1.0

Page {
    id: page
    tools: commonTools

    property string title : "PushUp — DLNA bridge"
    property string uuid : ""
    property alias deviceName: deviceLabel.text

    Image {
        id: pageHeader
        anchors {
            top: page.top
            left: page.left
            right: page.right
        }

        height: parent.width < parent.height ? 72 : 46
        width: parent.width
        source: "image://theme/meegotouch-view-header-fixed" + (theme.inverted ? "-inverted" : "")
        z: 1

        Label {
            id: header
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 16
            }
            platformStyle: LabelStyle {
                fontFamily: "Nokia Pure Text Light"
                fontPixelSize: 32
            }
            text: page.title
        }
    }

    Image {
        id: deviceIcon
        anchors.top: pageHeader.bottom
        anchors.topMargin: 100
        anchors.horizontalCenter: parent.horizontalCenter
        width: 128
        height: 128
        source: "qrc:/video-display.png"
        MouseArea {
            anchors.fill: parent
            onClicked: deviceSheet.open()

            Connections {
                target: deviceSheet
                onAccepted: {
                    console.log("Setting new source: " + deviceSheet.icon)
                    deviceIcon.source = deviceSheet.icon
                    uuid = deviceSheet.uuid
                    deviceName = deviceSheet.deviceName
                }
            }
        }
    }

    Label {
        id: deviceLabel
        text: qsTr("Select renderer")
        anchors.top: deviceIcon.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }

    PushUpController {
        id: controller
    }

    Button {
        id: toggleButton
        anchors.top: deviceLabel.bottom
        anchors.topMargin: 40
        anchors.horizontalCenter: parent.horizontalCenter
        checked: false
        checkable: true
        text: checked ? qsTr("Unshare phone audio") : qsTr("Share phone audio")
        enabled: uuid != ""
        onCheckedChanged: {
            if (checked) {
                controller.push("korva://system-audio", uuid);
            }
        }
    }
}
