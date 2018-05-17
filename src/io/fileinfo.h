/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_IO_FILEINFO_H
#define KBIBTEX_IO_FILEINFO_H

#include "kbibtexio_export.h"

#include <QSet>
#include <QUrl>
#include <QMimeDatabase>
#include <QMimeType>
#include <QSharedPointer>

class Entry;

class KBIBTEXIO_EXPORT FileInfo
{
public:
    static const QString mimetypeOctetStream;
    static const QString mimetypeHTML;
    static const QString mimetypeBibTeX;
    static const QString mimetypeRIS;
    static const QString mimetypePDF;

    enum TestExistence {
        TestExistenceYes, ///< Test if file exists
        TestExistenceNo ///< Skip test if file exists
    };

    /**
     * Finds a QMimeType with the given url.
     * Tries to guess a file's mime type by its extension first,
     * but falls back to QMimeType's mimeTypeForName if that does
     * not work. Background: If a HTTP or WebDAV server claims
     * that a .bib file is of mime type application/octet-stream,
     * QMimeType::mimeTypeForName will keep that assessment
     * instead of inspecting the file extension.
     *
     * @see QMimeType::mimeTypeForName
     * @param url Url to analyze
     * @return Guessed mime type
     */
    static QMimeType mimeTypeForUrl(const QUrl &url);

    /**
     * Find all file or URL references in the given text. Found filenames or
     * URLs are appended to the addTo list (duplicates are avoided).
     * Different test may get performed depending of the test for existence
     * of a potential file should be checked or not checked or if this matter
     * is undecided/irrelevant (recommended default case). For the test of
     * existence, baseDirectory is used to resolve relative paths.
     * @param text text to scan for filenames or URLs
     * @param testExistence shall be tested for file existence?
     * @param baseDirectory base directory for tests on relative path names
     * @param addTo add found URLs/filenames to this list
     */
    static void urlsInText(const QString &text, const TestExistence testExistence, const QString &baseDirectory, QSet<QUrl> &addTo);

    /**
     * Find all file or URL references in the given entry. Found filenames or
     * URLs are appended to the addTo list (duplicates are avoided).
     * Different test may get performed depending of the test for existence
     * of a potential file should be checked or not checked or if this matter
     * is undecided/irrelevant (recommended default case). For the test of
     * existence, bibTeXUrl is used to resolve relative paths.
     * @param entry entry to scan for filenames or URLs
     * @param bibTeXUrl base directory/URL for tests on relative path names
     * @param testExistence shall be tested for file existence?
     * @return list of found URLs/filenames (duplicates are avoided)
     */
    static QSet<QUrl> entryUrls(const QSharedPointer<const Entry> &entry, const QUrl &bibTeXUrl, TestExistence testExistence);

    /**
     * Load the given PDF file and return the contained plain text.
     * Makes use of Poppler to load and parse the file. All text
     * will be cached and loaded from cache if possible.
     * @param pdfFilename PDF file to load and extract text from
     * @return extracted plain text, either directly from PDF file or from cache OR QString() if there was an error
     */
    static QString pdfToText(const QString &pdfFilename);

    static QString doiUrlPrefix();

protected:
    FileInfo();

private:
    static void extractPDFTextToCache(const QString &pdfFilename, const QString &cacheFilename);
};

#endif // KBIBTEX_IO_FILEINFO_H
