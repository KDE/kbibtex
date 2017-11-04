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

#ifndef KBIBTEX_GUI_FINDPDFUI_H
#define KBIBTEX_GUI_FINDPDFUI_H

#include "kbibtexgui_export.h"

#include <QWidget>
#include <QLabel>

#include "entry.h"
#include "findpdf.h"

class QListView;

class FindPDF;

/**
 * A user interface too @see FindPDF
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT FindPDFUI : public QWidget
{
    Q_OBJECT

public:
    ~FindPDFUI() override;

    /**
     * Show a modal dialog that allows the user to start searching for PDF files,
     * and from the set of found results to select (1) which PDF files to download
     * and keep, (2) which URLs to PDF files to keep (no file downloading) and (3)
     * which PDF files to ignore.
     *
     * @param entry
     * @param bibtexFile
     * @param parent
     */
    static void interactiveFindPDF(Entry &entry, const File &bibtexFile, QWidget *parent);

signals:
    void resultAvailable(bool);

public slots:
    /**
     * Abort a running search.
     */
    void stopSearch();
    void abort();

protected:
    FindPDFUI(Entry &entry, QWidget *parent);

    void apply(Entry &entry, const File &bibtexFile);

private:
    class Private;
    Private *const d;

private slots:
    void searchFinished();
    void searchProgress(int visitedPages, int runningJobs, int foundDocuments);
};

#endif // KBIBTEX_GUI_FINDPDFUI_H
