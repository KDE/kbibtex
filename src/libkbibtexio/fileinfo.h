/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef KBIBTEX_IO_FILEINFO_H
#define KBIBTEX_IO_FILEINFO_H

#include "kbibtexio_export.h"

#include <QList>

#include <KUrl>

class Entry;

class KBIBTEXIO_EXPORT FileInfo
{
public:
    enum TestExistance {TestExistanceYes, TestExistanceNo};

    /**
     * Find all file or URL references in the given text. Found filenames or
     * URLs are appended to the addTo list (duplicates are avoided).
     * Different test may get performed depending of the test for existance
     * of a potential file should be checked or not checked or if this matter
     * is undecided/irrelevant (recommended default case). For the test of
     * existance, baseDirectory is used to resolve relative paths.
     * @param text text to scan for filenames or URLs
     * @param testExistance shall be tested for file existance?
     * @param baseDirectory base directory for tests on relative path names
     * @param addTo add found URLs/filenames to this list
     */
    static void urlsInText(const QString &text, TestExistance testExistance, const QString &baseDirectory, QList<KUrl> &addTo);

    /**
     * Find all file or URL references in the given entry. Found filenames or
     * URLs are appended to the addTo list (duplicates are avoided).
     * Different test may get performed depending of the test for existance
     * of a potential file should be checked or not checked or if this matter
     * is undecided/irrelevant (recommended default case). For the test of
     * existance, bibTeXUrl is used to resolve relative paths.
     * @param entry entry to scan for filenames or URLs
     * @param bibTeXUrl base directory/URL for tests on relative path names
     * @param testExistance shall be tested for file existance?
     * @return list of found URLs/filenames (duplicates are avoided)
     */
    static QList<KUrl> entryUrls(const Entry *entry, const KUrl &bibTeXUrl, TestExistance testExistance);

    /**
     * Load the given PDF file and return the contained plain text.
     * Makes use of Poppler to load and parse the file. All text
     * will be cached and loaded from cache if possible.
     * @param pdfFilename PDF file to load and extract text from
     * @return extracted plain text, either directly from PDF file or from cache OR QString::null if there was an error
     */
    static QString pdfToText(const QString& pdfFilename);

    static QString doiUrlPrefix();

protected:
    FileInfo();
};

#endif // KBIBTEX_IO_FILEINFO_H
