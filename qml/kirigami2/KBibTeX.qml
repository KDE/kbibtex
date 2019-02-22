/***************************************************************************
 *   Copyright (C) 2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de>      *
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

import QtQuick 2.7
import org.kde.kirigami 2.6 as Kirigami
import kirigami2.kbibtex 1.0
import "pages"

Kirigami.ApplicationWindow {
    id: root
    title: "KBibTeX/Kirigami2"

    SortedBibliographyModel {
        id: bibliographyModel
    }

    globalDrawer: Kirigami.GlobalDrawer {
        title: "KBibTeX"

        actions: [
            Kirigami.Action {
                text: "New Search"
                onTriggered: {
                    pageStack.clear()
                    pageStack.push(searchForm)
                }
            },
            Kirigami.Action {
                text: "About"
                onTriggered: {
                    pageStack.clear()
                    pageStack.push(aboutPage)
                }
            }
        ]
    }

    Component {
        id: resultPage
        BibliographyListView {
        }
    }

    Component {
        id: searchForm
        SearchForm {
        }
    }

    Kirigami.AboutPage {
        id: aboutPage
        aboutData: {
            "displayName" : "KBibTeX/Kirigami2",
            "shortDescription" : "A Kirigami2-based version of KBibTeX",
            "homepage": "https://userbase.kde.org/KBibTeX",
            "version": "0.1",
            "licenses": [{
                "name": "GPL v2 or later",
                "text": "This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.",
                "spdx": "GPL-2.0-or-later"
            }],
            "copyrightStatement": "© 2019 Thomas Fischer and others",
            "authors": [{
                "name": "Thomas Fischer",
                "emailAddress": "fischer@unix-ag.uni-kl.de"
            }],
            "otherText": ""
        }
    }
}
