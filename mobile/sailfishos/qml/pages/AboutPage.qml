/***************************************************************************
 *   Copyright (C) 2016-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
                    //% "About BibSearch"
                    title: qsTrId("bibsearch-application-about")
                }

                Image {
                    source: "qrc:/icons/128/harbour-bibsearch.png"
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                    }
                }

                Label {
                    //% "Version %1"
                    text: qsTrId("bibsearch-version").arg("0.6")
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                    }
                }

                Label {
                    //% "\u00a9 2016\u20132019 Thomas Fischer"
                    text: qsTrId("bibsearch-copyright-line")
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
                    //% "This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version."
                    text: qsTrId("bibsearch-under-gpl")
                    width: parent.width - 2 * x
                    x: Theme.horizontalPageMargin
                    font.pointSize: Theme.fontSizeTiny
                    wrapMode: Text.WordWrap
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    //% "Project Homepage"
                    text: qsTrId("bibsearch-link-label-project-hompage")
                    onClicked: {
                        Qt.openUrlExternally("https://gitlab.com/tfischer/BibSearch")
                    }
                }

                Label {
                    //% "This program is sharing its code base with KBibTeX, the bibliography editor using KDE technology."
                    text: qsTrId("bibsearch-based-on-kbibtex")
                    width: parent.width - 2 * x
                    x: Theme.horizontalPageMargin
                    font.pointSize: Theme.fontSizeTiny
                    wrapMode: Text.WordWrap
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    //% "KBibTeX's Homepage"
                    text: qsTrId("bibsearch-link-label-kbibtex-homepage")
                    onClicked: {
                        Qt.openUrlExternally("https://userbase.kde.org/KBibTeX")
                    }
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    //% "Git Repository"
                    text: qsTrId("bibsearch-link-label-git-repository")
                    onClicked: {
                        Qt.openUrlExternally("https://cgit.kde.org/kbibtex.git")
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
                //% "Report Issue"
                text: qsTrId("bibsearch-link-label-report-issue")
                onClicked: {
                    Qt.openUrlExternally("https://gitlab.com/tfischer/BibSearch/issues")
                }
            }
        }
    }
}
