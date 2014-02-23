/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef KBIBTEX_NETWORKING_ASSOCIATEDFILES_H
#define KBIBTEX_NETWORKING_ASSOCIATEDFILES_H

#include <QUrl>

#include "entry.h"

class QWidget;

class File;

#include "kbibtexnetworking_export.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT AssociatedFiles
{
public:
    enum PathType {ptAbsolute = 0, ptRelative = 1};
    enum RenameOperation {roKeepName = 0, roEntryId = 1};
    enum MoveCopyOperation {mcoCopy = 0, mcoMove = 1 };

    static bool urlIsLocal(const QUrl &url);
    static QString relativeFilename(const QUrl &document, const QUrl &baseUrl);
    static QString absoluteFilename(const QUrl &document, const QUrl &baseUrl);

    static QString associateDocumentURL(const QUrl &document, QSharedPointer<Entry> entry, File *bibTeXFile, PathType pathType);
    static QString associateDocumentURL(const QUrl &document, File *bibTeXFile, PathType pathType);
    static QUrl copyDocument(const QUrl &document, QSharedPointer<Entry> entry, File *bibTeXFile, RenameOperation renameOperation, MoveCopyOperation moveCopyOperation, QWidget *widget);
};

#endif // KBIBTEX_NETWORKING_ASSOCIATEDFILES_H
