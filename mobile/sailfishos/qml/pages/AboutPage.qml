/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2016-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: aboutPage
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        contentWidth: parent.width

        VerticalScrollDecorator {
        }

        Item {
            id: column
            width: parent.width
            height: childrenRect.height

            Column {
                width: parent.width
                spacing: Theme.horizontalPageMargin

                PageHeader {
                    id: header
                    title: qsTr("About BibSearch")
                }

                Image {
                    source: "qrc:/icons/172/harbour-bibsearch.png"
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                    }
                }

                Label {
                    text: qsTr("Version %1").arg("0.6")
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                    }
                }

                Label {
                    text: qsTr("(C) 2016--2020 Thomas Fischer")
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                    }
                }

                Rectangle {
                    height: Theme.horizontalPageMargin
                    width: parent.width
                    opacity: 0.0
                }

                Label {
                    text: qsTr("This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.")
                    width: parent.width - 2 * x
                    x: Theme.horizontalPageMargin
                    font.pointSize: Theme.fontSizeTiny
                    wrapMode: Text.WordWrap
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Project Homepage")
                    onClicked: {
                        Qt.openUrlExternally("https://gitlab.com/tfischer/BibSearch")
                    }
                }

                Label {
                    text: qsTr("This program is sharing its code base with KBibTeX, the bibliography editor using KDE technology.")
                    width: parent.width - 2 * x
                    x: Theme.horizontalPageMargin
                    font.pointSize: Theme.fontSizeTiny
                    wrapMode: Text.WordWrap
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("KBibTeX's Homepage")
                    onClicked: {
                        Qt.openUrlExternally("https://userbase.kde.org/KBibTeX")
                    }
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("Git Repository")
                    onClicked: {
                        Qt.openUrlExternally("https://invent.kde.org/office/kbibtex")
                    }
                }

                Rectangle {
                    height: 1
                    width: parent.width
                    opacity: 0.0
                }
            }
        }

        PullDownMenu {
            MenuItem {
                id: menuItemIssueTracker
                text: qsTr("Report Issue")
                onClicked: {
                    Qt.openUrlExternally("https://gitlab.com/tfischer/BibSearch/issues")
                }
            }
        }
    }
}
