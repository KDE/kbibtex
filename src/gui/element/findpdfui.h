/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <QWidget>
#include <QLabel>

#include <Entry>

#include "kbibtexgui_export.h"

class QListView;

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
     * @param entry Entry for which PDF files are to be searched for
     * @param bibtexFile Bibliography file to get a 'start URL' from it
     * @param parent Parent widget for the graphical user interface
     * @return True if the provided entry got modified, else False
     */
    static bool interactiveFindPDF(Entry &entry, const File &bibtexFile, QWidget *parent);

Q_SIGNALS:
    void resultAvailable(bool);

public Q_SLOTS:
    /**
     * Abort a running search.
     */
    void stopSearch();

protected:
    FindPDFUI(Entry &entry, QWidget *parent);

    bool apply(Entry &entry, const File &bibtexFile);

private:
    class Private;
    Private *const d;

private Q_SLOTS:
    void searchFinished();
    void searchProgress(int visitedPages, int runningJobs, int foundDocuments);
};

#endif // KBIBTEX_GUI_FINDPDFUI_H
