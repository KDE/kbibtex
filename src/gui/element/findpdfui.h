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
    FindPDFUI(Entry &entry, QWidget *parent);
    ~FindPDFUI();

    static void interactiveFindPDF(Entry &entry, const File &bibtexFile, QWidget *parent);
    void apply(Entry &entry, const File &bibtexFile);

signals:
    void resultAvailable(bool);

private:
    class Private;
    Private *const d;

private slots:
    void searchFinished();
    void searchProgress(int visitedPages, int runningJobs, int foundDocuments);
};

#endif // KBIBTEX_GUI_FINDPDFUI_H
