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

Sheet {
    acceptButtonText: qsTr("Select")
    rejectButtonText: qsTr("Cancel")

    property url icon : ""
    property string uuid : ""
    property string deviceName : ""

    content: ListView {
        id: listView
        model: PushUpDeviceModel {}
        highlight: ActiveSelection {}
        anchors.fill: parent
        height: 200
        width: parent.width
        delegate: Item {
            width: parent.width
            height: 88
            Image {
                id: deviceIcon
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 10
                width: 64
                height: 64
                fillMode: Image.PreserveAspectFit
                smooth: true
                source: decoration
            }

            Label {
                anchors.verticalCenter: deviceIcon.verticalCenter
                anchors.left: deviceIcon.right
                anchors.leftMargin: 10
                text: display
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    listView.currentIndex = index
                    icon = decoration
                    deviceName = display
                    uuid = edit
                }
            }
        }

        onCurrentIndexChanged: {
            getButton("acceptButton").enabled = currentIndex != -1;
        }

        ScrollDecorator {
            flickableItem: listView
        }
    }

    Component.onCompleted: {
        listView.currentIndex = -1
        getButton("acceptButton").enabled = listView.currentIndex != -1;
    }
}
