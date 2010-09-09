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

#include <QApplication>
#include <QClipboard>
#include <QBuffer>

#include <bibtexfileview.h>
#include <bibtexfilemodel.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <file.h>
#include "clipboard.h"

class Clipboard::ClipboardPrivate
{
private:
    Clipboard *parent;

public:
    BibTeXFileView *bibTeXFileView;

    ClipboardPrivate(BibTeXFileView *bfv, Clipboard *p)
            : parent(p), bibTeXFileView(bfv) {
        // TODO
    }
};

Clipboard::Clipboard(BibTeXFileView *bibTeXFileView)
        : QObject(bibTeXFileView), d(new ClipboardPrivate(bibTeXFileView, this))
{

}

void Clipboard::cut()
{
    copy();
    d->bibTeXFileView->selectionDelete();
}

void Clipboard::copy()
{
    QModelIndexList mil = d->bibTeXFileView->selectionModel()->selectedRows();
    File *file = new File();
    for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
        file->append(d->bibTeXFileView->bibTeXModel()->element(d->bibTeXFileView->sortFilterProxyModel()->mapToSource(*it).row()));
    }

    FileExporterBibTeX exporter;
    QBuffer buffer(d->bibTeXFileView);
    buffer.open(QBuffer::WriteOnly);
    exporter.save(&buffer, file);
    buffer.close();

    buffer.open(QBuffer::ReadOnly);
    QTextStream ts(&buffer);
    QString text = ts.readAll();
    buffer.close();

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void Clipboard::copyReferences()
{
    QStringList references;
    QModelIndexList mil = d->bibTeXFileView->selectionModel()->selectedRows();
    for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
        Entry *entry = dynamic_cast<Entry*>(d->bibTeXFileView->bibTeXModel()->element(d->bibTeXFileView->sortFilterProxyModel()->mapToSource(*it).row()));
        if (entry != NULL)
            references << entry->id();
    }

    if (!references.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(references.join(","));
    }
}

void Clipboard::paste()
{
    QItemSelectionModel *ism = d->bibTeXFileView->selectionModel();
    ism->clear();
    FileImporterBibTeX importer;
    QClipboard *clipboard = QApplication::clipboard();
    File *file = importer.fromString(clipboard->text());

    for (File::Iterator it = file->begin(); it != file->end(); ++it)
        d->bibTeXFileView->bibTeXModel()->insertRow(*it, d->bibTeXFileView->model()->rowCount());
}

