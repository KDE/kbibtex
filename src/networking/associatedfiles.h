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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KBIBTEX_NETWORKING_ASSOCIATEDFILES_H
#define KBIBTEX_NETWORKING_ASSOCIATEDFILES_H

#include <QUrl>

#include <Entry>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

class QWidget;

class File;

/**
 * Given a remote or local filename/URL, this class will, (1) at the user's
 * discretion, move or copy this file next to the bibliography's file into
 * the same directory, (2) rename the copied file (again, at the user's
 * discretion) to either match the corresponding entry's id or follow a name
 * provided by the user and (3) modify the entry to include a reference
 * (either relative or absolute path) to the newly moved/copied file or its
 * original filename/URL.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT AssociatedFiles
{
public:
    enum PathType {
        ptAbsolute = 0, ///< Use absolute filenames/paths if possible
        ptRelative = 1 ///< Use relative filenames/paths if possible
    };
    enum RenameOperation {
        roKeepName = 0, ///< Do not rename a file
        roEntryId = 1, ///< Rename the file following the entry's id
        roUserDefined = 2 ///< Rename after a string provided by the user
    };
    enum MoveCopyOperation {
        mcoNoCopyMove = 0, ///< Do not move or copy a file, use a reference only
        mcoCopy = 1, ///< Copy the file next to the bibiliograpy file
        mcoMove = 2 /// Same as copy, but delete original
    };

    /**
     * Test if a given URL points to a local file or a remote location.
     * @param url URL to test
     * @return true if the URL points to a local file
     */
    static bool urlIsLocal(const QUrl &url);

    /**
     * Translate a given URL of a document (e.g. a PDF file) to a string
     * representation pointing to the relative location of this document.
     * A "base URL", i.e. the bibliography's file location has to provided to
     * calculate the relative location of the document.
     * "Upwards relativity" (i.e. paths containing "..") is not supported for this
     * functions output; in this case, an absolute path will be generated as fallback.
     * @param document The document's URL
     * @param baseUrl The base URL
     * @return The document URL's string representation relative to the base URL
     */
    static QString relativeFilename(const QUrl &document, const QUrl &baseUrl);
    /**
     * Translate a given URL of a document (e.g. a PDF file) to a string
     * representation pointing to the absolute location of this document.
     * A "base URL", i.e. the bibliography's file location may be provided to
     * resolve relative document URLs
     * @param document The document's URL
     * @param baseUrl The base URL
     * @return The document URL's string representation in absolute form
     */
    static QString absoluteFilename(const QUrl &document, const QUrl &baseUrl);

    static QString associateDocumentURL(const QUrl &document, QSharedPointer<Entry> &entry, const File *bibTeXFile, PathType pathType, const bool dryRun = false);
    static QString associateDocumentURL(const QUrl &document, const File *bibTeXFile, PathType pathType);
    static QUrl copyDocument(const QUrl &document, const QString &entryId, const File *bibTeXFile, RenameOperation renameOperation, MoveCopyOperation moveCopyOperation, QWidget *widget, const QString &userDefinedFilename = QString(), const bool dryRun = false);
};

#endif // KBIBTEX_NETWORKING_ASSOCIATEDFILES_H
