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
import org.kde.kirigami 2.4 as Kirigami

Kirigami.ScrollablePage {
    id: resultPage

    implicitWidth: Kirigami.Units.gridUnit * 20
    background: Rectangle {
        color: Kirigami.Theme.backgroundColor
    }

    ListView {
        id: bibliographyListView
        model: bibliographyModel
        focus: true

        delegate: Kirigami.BasicListItem {
            id: delegate
            width: parent.width

            label: title
        }
    }
}
