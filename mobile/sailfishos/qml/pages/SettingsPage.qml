/***************************************************************************
 *   Copyright (C) 2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de>      *
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
import harbour.bibsearch 1.0

Page {
    id: settingsPage
    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height
        contentWidth: parent.width

        VerticalScrollDecorator {
        }

        Column {
            id: column
            width: parent.width

            PageHeader {
                //% "Settings"
                title: qsTrId("settings-title")
            }

            ComboBox {
                id: sortOrder
                //% "Sort Order"
                label: qsTrId("label-sort-order")
                currentIndex: bibliographyModel.sortOrder
                menu: ContextMenu {
                    Repeater {
                        model: bibliographyModel.humanReadableSortOrder()
                        MenuItem {
                            text: modelData
                        }
                    }
                }
                onCurrentItemChanged: {
                    bibliographyModel.sortOrder = currentIndex
                }
            }

            ValueButton {
                //% "Search Engines"
                label: qsTrId("label-search-engines")
                value: searchEngineList.searchEngineCount === 0
                       //% "None selected"
                       ? qsTrId("selected-count-none")
                       //% "%1 selected"
                       : qsTrId("selected-count-numarg").arg(searchEngineList.searchEngineCount)
                description: searchEngineList.searchEngineCount === 0
                             //% "At least one search engine must be selected."
                             ? qsTrId("label-selected-atleastone")
                             : searchEngineList.humanReadableSearchEngines()

                onClicked: {
                    pageStack.push("SearchEngineListView.qml")
                }
            }
        }
    }
}
