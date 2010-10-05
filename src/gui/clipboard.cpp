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
#include <QMouseEvent>
#include <QDrag>

#include <KDebug>

#include <bibtexeditor.h>
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
    BibTeXEditor *bibTeXEditor;
    QPoint previousPosition;

    ClipboardPrivate(BibTeXEditor *be, Clipboard *p)
            : parent(p), bibTeXEditor(be) {
        // TODO
    }

    QString selectionToText() {
        QModelIndexList mil = bibTeXEditor->selectionModel()->selectedRows();
        File *file = new File();
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
            file->append(bibTeXEditor->bibTeXModel()->element(bibTeXEditor->sortFilterProxyModel()->mapToSource(*it).row()));
        }

        FileExporterBibTeX exporter;
        QBuffer buffer(bibTeXEditor);
        buffer.open(QBuffer::WriteOnly);
        exporter.save(&buffer, file);
        buffer.close();

        buffer.open(QBuffer::ReadOnly);
        QTextStream ts(&buffer);
        QString text = ts.readAll();
        buffer.close();

        return text;
    }

    void insertText(const QString &text) {
        QItemSelectionModel *ism = bibTeXEditor->selectionModel();
        ism->clear();
        FileImporterBibTeX importer;
        File *file = importer.fromString(text);

        for (File::Iterator it = file->begin(); it != file->end(); ++it)
            bibTeXEditor->bibTeXModel()->insertRow(*it, bibTeXEditor->model()->rowCount());
    }
};

Clipboard::Clipboard(BibTeXEditor *bibTeXEditor)
        : QObject(bibTeXEditor), d(new ClipboardPrivate(bibTeXEditor, this))
{
    connect(bibTeXEditor, SIGNAL(editorMouseEvent(QMouseEvent*)), this, SLOT(editorMouseEvent(QMouseEvent*)));
    connect(bibTeXEditor, SIGNAL(editorDragEnterEvent(QDragEnterEvent*)), this, SLOT(editorDragEnterEvent(QDragEnterEvent*)));
    connect(bibTeXEditor, SIGNAL(editorDragMoveEvent(QDragMoveEvent*)), this, SLOT(editorDragMoveEvent(QDragMoveEvent*)));
    connect(bibTeXEditor, SIGNAL(editorDropEvent(QDropEvent*)), this, SLOT(editorDropEvent(QDropEvent*)));
    bibTeXEditor->setAcceptDrops(true);
}

void Clipboard::cut()
{
    copy();
    d->bibTeXEditor->selectionDelete();
}

void Clipboard::copy()
{
    QString text = d->selectionToText();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void Clipboard::copyReferences()
{
    QStringList references;
    QModelIndexList mil = d->bibTeXEditor->selectionModel()->selectedRows();
    for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
        Entry *entry = dynamic_cast<Entry*>(d->bibTeXEditor->bibTeXModel()->element(d->bibTeXEditor->sortFilterProxyModel()->mapToSource(*it).row()));
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
    QClipboard *clipboard = QApplication::clipboard();
    d->insertText(clipboard->text());
}


void Clipboard::editorMouseEvent(QMouseEvent *event)
{
    if (!(event->buttons()&Qt::LeftButton))
        return;

    if (d->previousPosition.x() > -1 && (event->pos() - d->previousPosition).manhattanLength() >= QApplication::startDragDistance()) {
        QString text = d->selectionToText();

        QDrag *drag = new QDrag(d->bibTeXEditor);
        QMimeData *mimeData = new QMimeData();
        QByteArray data = text.toAscii();
        mimeData->setData("text/plain", data);
        drag->setMimeData(mimeData);

        Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
        kDebug() << "dropAction = " << dropAction;
        // Q_ASSERT_X(dropAction == Qt::CopyAction, "void Clipboard::editorMouseEvent(QMouseEvent *event)", "Drag'n'drop is not the expected copy operation");
    }

    d->previousPosition = event->pos();
}

void Clipboard::editorDragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void Clipboard::editorDragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasText())
        event->acceptProposedAction();
}

void Clipboard::editorDropEvent(QDropEvent *event)
{
    QString text = event->mimeData()->text();

    if (!text.isEmpty())
        d->insertText(text);
}
