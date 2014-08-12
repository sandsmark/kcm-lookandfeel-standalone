/*
   Copyright (c) 2014 Marco Martin <mart@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.0 as QtControls
import org.kde.kquickcontrolsaddons 2.0
import QtQuick.Controls.Private 1.0
//We need units from it
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

Rectangle {
    width: units.gridUnit * 20
    height: units.gridUnit * 20
    color: syspal.window

    SystemPalette {id: syspal}
    QtControls.ScrollView {
        anchors.fill: parent
        GridView {
            id: grid
            model: kcm.lookAndFeelModel
            cellWidth: Math.floor(grid.width / Math.max(Math.floor(grid.width / (units.gridUnit*12)), 3))
            cellHeight: cellWidth / 1.6

            delegate: Rectangle {
                width: grid.cellWidth
                height: grid.cellHeight
                Connections {
                    target: kcm
                    onSelectedPluginChanged: {
                        if (kcm.selectedPlugin == model.pluginName) {
                            makeCurrentTimer.pendingIndex = index
                        }
                    }
                }
                Component.onCompleted: {
                    if (kcm.selectedPlugin == model.pluginName) {
                        makeCurrentTimer.pendingIndex = index
                    }
                }
                QIconItem {
                    id: icon
                    anchors.centerIn: parent
                    width: units.iconSizes.large
                    height: width
                    icon: "view-preview"
                }
                QtControls.Label {
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        top: icon.bottom
                        topMargin: units.gridUnit
                    }
                    color: "gray"
                    text: model.display
                }
                Image {
                    anchors.fill: parent
                    source: model.screenshot
                }
                Rectangle {
                    opacity: grid.currentIndex == index ? 1.0 : 0
                    anchors.fill: parent
                    border.width: units.smallSpacing * 2
                    border.color: syspal.highlight
                    color: "transparent"
                    Behavior on opacity {
                        PropertyAnimation {
                            duration: units.longDuration
                            easing.type: Easing.OutQuad
                        }
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        grid.currentIndex = index
                        kcm.selectedPlugin = model.pluginName
                    }
                    Timer {
                        interval: 1000 // FIXME TODO: Use platform value for tooltip activation delay.

                        running: parent.containsMouse && !parent.pressedButtons

                        onTriggered: {
                            Tooltip.showText(parent, Qt.point(parent.mouseX, parent.mouseY), model.display);
                        }
                    }
                }
            }
            Timer {
                id: makeCurrentTimer
                interval: 100
                repeat: false
                property int pendingIndex
                onPendingIndexChanged: makeCurrentTimer.restart()
                onTriggered: {
                    grid.currentIndex = pendingIndex
                }
            }
        }
    }
}
