/***************************************************************************
 *   Copyright (C) 2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
    id: searchEngineListViewPage
    allowedOrientations: Orientation.All

    SilicaFlickable {
        id: resultList
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content
            width: parent.width

            PageHeader {
                title: "Available Search Engines"
            }

            SilicaListView {
                id: searchEngineListView
                model: searchEngineList
                width: parent.width
                height: childrenRect.height

                delegate: TextSwitch {
                    width: parent.width
                    text: label
                    Component.onCompleted: checked = engineEnabled
                    onCheckedChanged: engineEnabled = checked
                }
            }
        }
    }
}
