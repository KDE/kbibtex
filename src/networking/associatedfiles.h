/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
    enum class PathType {
        Absolute, ///< Use absolute filenames/paths if possible
        Relative ///< Use relative filenames/paths if possible
    };
    enum class RenameOperation {
        KeepName, ///< Do not rename a file
        EntryId, ///< Rename the file following the entry's id
        UserDefined ///< Rename after a string provided by the user
    };
    enum class MoveCopyOperation {
        None, ///< Do not move or copy a file, use a reference only
        Copy, ///< Copy the file next to the bibiliograpy file
        Move /// Same as copy, but delete original
    };

    /**
     * Based on a given URL to an external document, compute an URL used for association
     * and insert it into the given entry, either as local file or as URL.
     *
     * @param documentUrl URL to a document like 'http://www.example.com/publication.pdf'
     * @param entry bibliography entry where the URL is to be associated with
     * @param bibTeXFile valid bibliography, preferably with property 'File::Url' set
     * @param pathType request either a relative or an absolute path
     * @return the computed URL string
     */
    static QString insertUrl(const QUrl &documentUrl, QSharedPointer<Entry> &entry, const File *bibTeXFile, PathType pathType);

    /**
     * Compute how the URL string to be associated to a bibliographic entry may look
     * like for a given document URL, a given bibliography, and whether the URL string
     * should be preferably relative or absolute.
     * @param documentUrl URL to a document like 'http://www.example.com/publication.pdf'
     * @param bibTeXFile valid bibliography, preferably with property 'File::Url' set
     * @param pathType request either a relative or an absolute path
     * @return the computed URL string
     */
    static QString computeAssociateString(const QUrl &documentUrl, const File *bibTeXFile, PathType pathType);

    /**
     * For a given (remote) source URL and given various information such as which
     * bibliographic entry and file the local copy will be associated with, determine
     * a destination URL where the source document may be copied to.
     * This function will neither modify the bibliographic entry or file, nor do the
     * actual copying.
     *
     * @param sourceUrl The remote location of the document
     * @param entryId the identifier of the bibliography entry
     * @param bibTeXFile the bibliographic file
     * @param renameOperation what type of renaming is requested
     * @param userDefinedFilename an optional custom basename
     * @return A pair of URLs: refined source URL and computed destination URL
     */
    static QPair<QUrl, QUrl> computeSourceDestinationUrls(const QUrl &sourceUrl, const QString &entryId, const File *bibTeXFile, RenameOperation renameOperation, const QString &userDefinedFilename);

    static QUrl copyDocument(const QUrl &document, const QString &entryId, const File *bibTeXFile, RenameOperation renameOperation, MoveCopyOperation moveCopyOperation, QWidget *widget, const QString &userDefinedFilename = QString());

private:
    /**
     * Translate a given URL of a document (e.g. a PDF file) to a string
     * representation pointing to the relative location of this document.
     * A "base URL", i.e. the bibliography's file location has to be provided
     * in order to calculate the relative location of the document.
     * "Upwards relativity" (i.e. paths containing "..") is not supported for this
     * functions output; in this case, an absolute path will be generated as fallback.
     *
     * @param document The document's URL
     * @param baseUrl The base URL
     * @return The document URL's string representation relative to the base URL
     */
    static QString relativeFilename(const QUrl &document, const QUrl &baseUrl);
    /**
     * Translate a given URL of a document (e.g. a PDF file) to a string
     * representation pointing to the absolute location of this document.
     * A "base URL", i.e. the bibliography's file location may be provided to
     * resolve relative document URLs.
     *
     * @param document The document's URL
     * @param baseUrl The base URL
     * @return The document URL's string representation in absolute form
     */
    static QString absoluteFilename(const QUrl &document, const QUrl &baseUrl);
};

#endif // KBIBTEX_NETWORKING_ASSOCIATEDFILES_H
