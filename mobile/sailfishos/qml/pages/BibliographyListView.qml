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
    id: resultPage
    allowedOrientations: Orientation.All

    SilicaListView {
        id: bibliographyListView
        model: bibliographyModel

        spacing: Theme.paddingMedium
        anchors.fill: parent

        VerticalScrollDecorator {
            enabled: bibliographyListView.count > 0
        }

        ViewPlaceholder {
            enabled: bibliographyListView.count === 0
            text: bibliographyModel.busy ? qsTr("Waiting for results \u2026") : qsTr("Pull down to start a new search.")
        }

        delegate: BackgroundItem {
            id: delegate
            width: parent.width
            height: col.childrenRect.height

            Column {
                id: col
                width: parent.width - x
                x: Theme.horizontalPageMargin

                Label {
                    text: authorShort + " (" + year + ")"
                    width: parent.width
                    font.pointSize: Theme.fontSizeSmall
                    clip: true
                    color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    text: title
                    font.pointSize: Theme.fontSizeMedium
                    width: parent.width
                    clip: true
                    color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    visible: wherePublished.length > 0
                    text: wherePublished
                    width: parent.width
                    clip: true
                    font.pointSize: Theme.fontSizeExtraSmall
                    color: delegate.highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    opacity: 0.9
                    truncationMode: TruncationMode.Fade
                }
            }

            onClicked: pageStack.push("EntryView.qml", {
                                          author: author,
                                          title: title,
                                          wherePublished: wherePublished,
                                          year: year,
                                          url: url,
                                          doi: doi,
                                          foundVia: foundVia
                                      })
        }

        PullDownMenu {
            MenuItem {
                text: qsTr("About")
                onClicked: pageStack.push("AboutPage.qml")
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push("SettingsPage.qml")
            }
            MenuItem {
                text: qsTr("New Search")
                onClicked: pageStack.push("SearchForm.qml")
            }
        }
    }
}
