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

PageStackWindow {
    id: appWindow

    initialPage: mainPage

    MainPage {
        id: mainPage
    }

    DeviceSheet {
        id: deviceSheet
    }

    ToolBarLayout {
        id: commonTools

        ToolIcon {
            anchors.right: parent.right
            platformIconId: "toolbar-view-menu"
            onClicked: {
                appWindow.showToolBar = false;
                myMenu.open();
            }
        }
    }

    QueryDialog {
        id: aboutDialog
        titleText: "PushUp Control"
        message: "(C) 2012 Jens Georg"
    }

    Menu {
        id: myMenu
        visualParent: appWindow
        MenuLayout {
            MenuItem {
                text: qsTr("About")
                onClicked: {
                    aboutDialog.open();
                }
            }
        }
        onStatusChanged: {
            if (status === DialogStatus.Closed) {
                appWindow.showToolBar = true;
            }
        }
    }

    Component.onCompleted: theme.inverted = true
}
