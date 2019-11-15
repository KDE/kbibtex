/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_NETWORKING_FINDPDF_H
#define KBIBTEX_NETWORKING_FINDPDF_H

#include "kbibtexnetworking_export.h"

#include <QObject>
#include <QList>
#include <QSet>
#include <QUrl>

#include <Entry>

class QNetworkAccessManager;
class QNetworkReply;
class QTemporaryFile;

/**
 * Search known Internet resources (search engines) for PDF files
 * matching a given bibliography entry.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT FindPDF : public QObject
{
    Q_OBJECT

public:
    /// Used in a later stage (user interface, @see FindPDFUI);
    /// tells the system if ...
    enum class DownloadMode {
        No = 0, ///< Ignore this result item (no PDF file downloading)
        PDFfile = 1, ///< Download and store this PDF file in a user-specified location
        URLonly = 2 ///< Keep only the URL of the PDF; this URL will be inserted in the bib entry
    };

    /// Structure to store data about every found PDF (potential search hit)
    typedef struct {
        QUrl url; ///< Where has this PDF been found?
        QString textPreview; ///< Text extracted from the PDF file
        QTemporaryFile *tempFilename; ///< Local temporary copy
        float relevance; /// Assessment of relevance (useful for sorting results)
        DownloadMode downloadMode; /// User's preference what to do with this hit (default is NoDownload)
    } ResultItem;

    explicit FindPDF(QObject *parent = nullptr);
    ~FindPDF() override;

    /**
     * Initiate a search for PDF files matching a given entry.
     *
     * @param entry entry to search PDF files for
     * @return @c true if the search could be started @c false if another search is still running
     */
    bool search(const Entry &entry);

    /**
     * Once a search has been complete (signal @see finished),
     * this function allows to retrieve the collected results
     * @return @c After a search, list of results, @c before or during a search, an empty list
     */
    QList<ResultItem> results();

signals:
    /**
     * A search initiated by @see search has been finished.
     */
    void finished();

    /**
     * Some update on the ongoing search.
     * Just of eye candy, can be safely ignored if no visualization of progress is possible.
     *
     * @param visitedPages how many web pages have been visited
     * @param runningJobs how many download/search operations are running in parallel
     * @param foundDocuments how many PDF files have been found
     */
    void progress(int visitedPages, int runningJobs, int foundDocuments);

public slots:
    /**
     * Abort any running downloads.
     */
    void abort();

private slots:
    void downloadFinished();

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_NETWORKING_FINDPDF_H
