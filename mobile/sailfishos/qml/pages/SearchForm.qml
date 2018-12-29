/***************************************************************************
 *   Copyright (C) 2016-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
    id: searchForm
    allowedOrientations: Orientation.All

    SilicaFlickable {
        id: resultList
        anchors.fill: parent
        contentHeight: content.height

        function startSearching() {
            bibliographyModel.clear();
            bibliographyModel.startSearch(inputFreeText.text,
                                          inputTitle.text,
                                          inputAuthor.text);
            pop();
        }

        Column {
            id: content
            width: parent.width

            PageHeader {
                title: "Enter Search Parameters"
            }

            ValueButton {
                label: qsTr("Search Engines")
                value: searchEngineList.searchEngineCount === 0
                       ? qsTr("None selected")
                       : qsTr("%1 selected").arg(searchEngineList.searchEngineCount)
                description: searchEngineList.searchEngineCount === 0
                             ? qsTr("At least one search engine must be selected.")
                             : searchEngineList.humanReadableSearchEngines()

                onClicked: {
                    pageStack.push("SearchEngineListView.qml")
                }
            }

            TextField {
                id: inputFreeText
                label: qsTr("Free text")
                placeholderText: label
                width: parent.width
                focus: true
                enabled: searchEngineList.searchEngineCount > 0

                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-search"
                EnterKey.onClicked: resultList.startSearching()
            }

            TextField {
                id: inputTitle
                label: qsTr("Title")
                placeholderText: label
                width: parent.width
                enabled: searchEngineList.searchEngineCount > 0

                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-search"
                EnterKey.onClicked: resultList.startSearching()
            }

            TextField {
                id: inputAuthor
                label: qsTr("Author")
                placeholderText: label
                width: parent.width
                enabled: searchEngineList.searchEngineCount > 0

                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-search"
                EnterKey.onClicked: resultList.startSearching()
            }
        }

        PullDownMenu {
            MenuItem {
                id: menuItemStartSearching
                text: qsTr("Start Searching")
                enabled: (inputFreeText.text.length > 0
                          || inputTitle.text.length > 0
                          || inputAuthor.text.length > 0)
                         && searchEngineList.searchEngineCount > 0
                onClicked: resultList.startSearching()
            }

            visible: menuItemStartSearching.enabled
        }
    }
}
