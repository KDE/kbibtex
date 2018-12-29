/***************************************************************************
 *   Copyright (C) 2016-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
    id: entryView
    allowedOrientations: Orientation.All

    property string author: ""
    property string title: ""
    property string wherePublished: ""
    property string year: ""
    property string url: ""
    property string doi: ""
    property string foundVia: ""

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        Column {
            id: content
            x: Theme.horizontalPageMargin
            width: parent.width - x

            PageHeader {
                title: entryView.title
            }

            Label {
                text: qsTr("Author")
                color: Theme.highlightColor
                font.pointSize: Theme.fontSizeExtraSmall
            }
            Label {
                id: author
                text: entryView.author
                wrapMode: Text.WordWrap
                width: parent.width - Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                font.pointSize: Theme.fontSizeMedium
            }

            Label {
                text: qsTr("Title")
                color: Theme.highlightColor
                font.pointSize: Theme.fontSizeExtraSmall
            }
            Label {
                id: title
                text: entryView.title
                wrapMode: Text.WordWrap
                width: parent.width - Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                font.pointSize: Theme.fontSizeMedium
            }
            Label {
                visible: entryView.wherePublished.length > 0
                text: qsTr("Publication")
                color: Theme.highlightColor
                font.pointSize: Theme.fontSizeExtraSmall
            }
            Label {
                id: wherePublished
                text: entryView.wherePublished
                visible: entryView.wherePublished.length > 0
                wrapMode: Text.Wrap
                width: parent.width - Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                font.pointSize: Theme.fontSizeMedium
            }
            Label {
                text: qsTr("Year")
                color: Theme.highlightColor
                font.pointSize: Theme.fontSizeExtraSmall
            }
            Label {
                id: year
                text: entryView.year
                wrapMode: Text.WordWrap
                width: parent.width - Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                font.pointSize: Theme.fontSizeMedium
            }
            Label {
                visible: entryView.doi.length > 0 || entryView.url.length > 0
                text: entryView.doi.length > 0 ? qsTr("DOI") : qsTr("URL")
                color: Theme.highlightColor
                font.pointSize: Theme.fontSizeExtraSmall
            }
            Label {
                id: urlOrDoi
                visible: entryView.doi.length > 0 || entryView.url.length > 0
                text: entryView.doi.length > 0 ? entryView.doi : entryView.url
                wrapMode: Text.WordWrap
                width: parent.width - Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                font.pointSize: Theme.fontSizeMedium
            }
            Label {
                visible: entryView.foundVia.length > 0
                text: qsTr("Found via")
                color: Theme.highlightColor
                font.pointSize: Theme.fontSizeExtraSmall
            }
            Label {
                id: foundVia
                text: entryView.foundVia
                visible: entryView.foundVia.length > 0
                wrapMode: Text.Wrap
                width: parent.width - Theme.horizontalPageMargin
                color: Theme.secondaryHighlightColor
                font.pointSize: Theme.fontSizeMedium
            }
        }

        PullDownMenu {
            MenuItem {
                id: menuItemViewOnline
                text: qsTr("View Online")
                enabled: entryView.url.length > 0
                onClicked: {
                    Qt.openUrlExternally(url)
                }
            }

            visible: menuItemViewOnline.enabled
        }
    }
}
