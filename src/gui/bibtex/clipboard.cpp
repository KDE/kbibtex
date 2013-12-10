/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QBuffer>
#include <QMouseEvent>
#include <QDrag>

#include <KConfigGroup>
#include <KDebug>

#include "bibtexeditor.h"
#include "bibtexfilemodel.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"

const QString Clipboard::keyCopyReferenceCommand = QLatin1String("copyReferenceCommand");
const QString Clipboard::defaultCopyReferenceCommand = QLatin1String("");

class Clipboard::ClipboardPrivate
{
private:
    Clipboard *parent;

public:
    BibTeXEditor *bibTeXEditor;
    QPoint previousPosition;
    KSharedConfigPtr config;
    const QString configGroupName;

    ClipboardPrivate(BibTeXEditor *be, Clipboard *p)
            : parent(p), bibTeXEditor(be), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("General")) {
        // TODO
    }

    QString selectionToText() {
        QModelIndexList mil = bibTeXEditor->selectionModel()->selectedRows();
        File *file = new File();
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it)
            file->append(bibTeXEditor->bibTeXModel()->element(bibTeXEditor->sortFilterProxyModel()->mapToSource(*it).row()));

        FileExporterBibTeX exporter;
        exporter.setEncoding(QLatin1String("latex"));
        QBuffer buffer(bibTeXEditor);
        buffer.open(QBuffer::WriteOnly);
        exporter.save(&buffer, file);
        buffer.close();

        buffer.open(QBuffer::ReadOnly);
        QTextStream ts(&buffer);
        QString text = ts.readAll();
        buffer.close();

        /// clean up
        delete file;

        return text;
    }

    bool insertText(const QString &text, QSharedPointer<Element> element = QSharedPointer<Element>()) {
        /// First assumption: user dropped a piece of BibTeX code,
        /// use BibTeX importer to generate representation from plain text
        FileImporterBibTeX importer;
        File *file = importer.fromString(text);

        if (!file->isEmpty()) {
            BibTeXFileModel *bibTeXModel = bibTeXEditor->bibTeXModel();
            QSortFilterProxyModel *sfpModel = bibTeXEditor->sortFilterProxyModel();

            /// insert new elements one by one
            int startRow = bibTeXModel->rowCount(); ///< memorize row where insertion started
            for (File::Iterator it = file->begin(); it != file->end(); ++it)
                bibTeXModel->insertRow(*it, bibTeXEditor->model()->rowCount());
            int endRow = bibTeXModel->rowCount() - 1; ///< memorize row where insertion ended

            /// select newly inserted elements
            QItemSelectionModel *ism = bibTeXEditor->selectionModel();
            ism->clear();
            /// keep track of the insert element which is most upwards in the list when inserted
            QModelIndex minRowTargetModelIndex;
            /// highlight those rows in the editor which correspond to newly inserted elements
            for (int i = startRow; i <= endRow; ++i) {
                QModelIndex targetModelIndex = sfpModel->mapFromSource(bibTeXModel->index(i, 0));
                ism->select(targetModelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);

                /// update the most upward inserted element
                if (!minRowTargetModelIndex.isValid() || minRowTargetModelIndex.row() > targetModelIndex.row())
                    minRowTargetModelIndex = targetModelIndex;
            }
            /// scroll tree view to show top-most inserted element
            bibTeXEditor->scrollTo(minRowTargetModelIndex, QAbstractItemView::PositionAtTop);

            /// clean up
            delete file;
            /// return true if at least one element was inserted
            return startRow <= endRow;
        } else if (!element.isNull()) {
            /// Regular text seems to have been dropped on a specific element

            QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
            if (!entry.isNull()) {
                kDebug() << "Adding in entry" << entry->id();

                /// Check if text looks like an URL
                KUrl url(text);
                if (url.isValid()) {
                    /// Try to resolve relative URLs
                    QUrl fileUrl = bibTeXEditor->bibTeXModel()->bibTeXFile()->property(File::Url, QUrl()).toUrl();
                    if (url.isRelative() && fileUrl.isValid()) url = fileUrl.resolved(url);

                    // FIXME this code is redundant to findpdfui.cpp
                    bool alreadyContained = false;
                    const QString urlText = url.pathOrUrl();
                    kDebug() << "Adding url" << urlText;
                    for (QMap<QString, Value>::ConstIterator it = entry->constBegin(); !alreadyContained && it != entry->constEnd(); ++it) {
                        const QString key = it.key().toLower();
                        // FIXME this will terribly break if URLs in an entry's URL field are separated with semicolons
                        alreadyContained |= (key.startsWith(Entry::ftUrl) || key.startsWith(Entry::ftLocalFile)) && PlainTextValue::text(it.value()) == urlText;
                    }
                    if (!alreadyContained) {
                        Value value;
                        value.append(QSharedPointer<VerbatimText>(new VerbatimText(urlText)));
                        const QString field = url.isLocalFile() ? Entry::ftLocalFile : Entry::ftUrl;
                        kDebug() << "Adding to field" << field;
                        if (!entry->contains(field))
                            entry->insert(field, value);
                        else
                            for (int i = 2; i < 256; ++i) {
                                const QString keyName = QString("%1%2").arg(field).arg(i);
                                if (!entry->contains(keyName)) {
                                    entry->insert(keyName, value);
                                    break;
                                }
                            }
                    }
                    return true;
                }
            }
        } else
            kDebug() << "Don't know what to do with " << text;

        return false;
    }
};

Clipboard::Clipboard(BibTeXEditor *bibTeXEditor)
        : QObject(bibTeXEditor), d(new ClipboardPrivate(bibTeXEditor, this))
{
    connect(bibTeXEditor, SIGNAL(editorMouseEvent(QMouseEvent*)), this, SLOT(editorMouseEvent(QMouseEvent*)));
    connect(bibTeXEditor, SIGNAL(editorDragEnterEvent(QDragEnterEvent*)), this, SLOT(editorDragEnterEvent(QDragEnterEvent*)));
    connect(bibTeXEditor, SIGNAL(editorDragMoveEvent(QDragMoveEvent*)), this, SLOT(editorDragMoveEvent(QDragMoveEvent*)));
    connect(bibTeXEditor, SIGNAL(editorDropEvent(QDropEvent*)), this, SLOT(editorDropEvent(QDropEvent*)));
    bibTeXEditor->setAcceptDrops(!bibTeXEditor->isReadOnly());
}

Clipboard::~Clipboard()
{
    delete d;
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
        QSharedPointer<Entry> entry = d->bibTeXEditor->bibTeXModel()->element(d->bibTeXEditor->sortFilterProxyModel()->mapToSource(*it).row()).dynamicCast<Entry>();
        if (!entry.isNull())
            references << entry->id();
    }

    if (!references.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        QString text = references.join(",");

        KConfigGroup configGroup(d->config, d->configGroupName);
        const QString copyReferenceCommand = configGroup.readEntry(keyCopyReferenceCommand, defaultCopyReferenceCommand);
        if (!copyReferenceCommand.isEmpty())
            text = QString(QLatin1String("\\%1{%2}")).arg(copyReferenceCommand).arg(text);

        clipboard->setText(text);
    }
}

void Clipboard::paste()
{
    QClipboard *clipboard = QApplication::clipboard();
    d->insertText(clipboard->text(), d->bibTeXEditor->currentElement());
    d->bibTeXEditor->externalModification();
}


void Clipboard::editorMouseEvent(QMouseEvent *event)
{
    if (!(event->buttons()&Qt::LeftButton))
        return;

    if (d->previousPosition.x() > -1 && (event->pos() - d->previousPosition).manhattanLength() >= QApplication::startDragDistance()) {
        QString text = d->selectionToText();

        QDrag *drag = new QDrag(d->bibTeXEditor);
        QMimeData *mimeData = new QMimeData();
        QByteArray data = text.toLatin1();
        mimeData->setData("text/plain", data);
        drag->setMimeData(mimeData);

        drag->exec(Qt::CopyAction);
    }

    d->previousPosition = event->pos();
}

void Clipboard::editorDragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText() && !d->bibTeXEditor->isReadOnly())
        event->acceptProposedAction();
}

void Clipboard::editorDragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasText() && !d->bibTeXEditor->isReadOnly())
        event->acceptProposedAction();
}

void Clipboard::editorDropEvent(QDropEvent *event)
{
    QString text = event->mimeData()->text();

    if (!text.isEmpty() && !d->bibTeXEditor->isReadOnly()) {
        QSharedPointer<Element> element;
        /// Locate element drop was performed on (if dropped, and not some copy&paste)
        QModelIndex dropIndex = d->bibTeXEditor->indexAt(event->pos());
        if (dropIndex.isValid())
            element = d->bibTeXEditor->elementAt(dropIndex);
        if (element.isNull()) {
            /// Still invalid element? Use current one
            element = d->bibTeXEditor->currentElement();
        }

        d->insertText(text, element);
        d->bibTeXEditor->externalModification();
    }
}
