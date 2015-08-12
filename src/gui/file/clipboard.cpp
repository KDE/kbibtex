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

#include "clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QMouseEvent>
#include <QDrag>
#include <QDebug>

#include <KConfigGroup>
#include <KSharedConfig>

#include "fileview.h"
#include "filemodel.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"
#include "associatedfilesui.h"

const QString Clipboard::keyCopyReferenceCommand = QLatin1String("copyReferenceCommand");
const QString Clipboard::defaultCopyReferenceCommand = QLatin1String("");

class Clipboard::ClipboardPrivate
{
private:
    // UNUSED Clipboard *parent;

public:
    FileView *fileView;
    QPoint previousPosition;
    KSharedConfigPtr config;
    const QString configGroupName;

    ClipboardPrivate(FileView *fv, Clipboard */* UNUSED p*/)
        : /* UNUSED parent(p),*/ fileView(fv), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("General")) {
        /// nothing
    }

    QString selectionToText() {
        QModelIndexList mil = fileView->selectionModel()->selectedRows();
        File *file = new File();
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it)
            file->append(fileView->fileModel()->element(fileView->sortFilterProxyModel()->mapToSource(*it).row()));

        FileExporterBibTeX exporter;
        exporter.setEncoding(QLatin1String("latex"));
        QBuffer buffer(fileView);
        buffer.open(QBuffer::WriteOnly);
        const bool success = exporter.save(&buffer, file);
        buffer.close();
        if (!success) {
            delete file;
            return QString();
        }

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

        if (file != NULL && !file->isEmpty()) {
            FileModel *fileModel = fileView->fileModel();
            QSortFilterProxyModel *sfpModel = fileView->sortFilterProxyModel();

            /// insert new elements one by one
            int startRow = fileModel->rowCount(); ///< memorize row where insertion started
            for (File::Iterator it = file->begin(); it != file->end(); ++it)
                fileModel->insertRow(*it, fileView->model()->rowCount());
            int endRow = fileModel->rowCount() - 1; ///< memorize row where insertion ended

            /// select newly inserted elements
            QItemSelectionModel *ism = fileView->selectionModel();
            ism->clear();
            /// keep track of the insert element which is most upwards in the list when inserted
            QModelIndex minRowTargetModelIndex;
            /// highlight those rows in the editor which correspond to newly inserted elements
            for (int i = startRow; i <= endRow; ++i) {
                QModelIndex targetModelIndex = sfpModel->mapFromSource(fileModel->index(i, 0));
                ism->select(targetModelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);

                /// update the most upward inserted element
                if (!minRowTargetModelIndex.isValid() || minRowTargetModelIndex.row() > targetModelIndex.row())
                    minRowTargetModelIndex = targetModelIndex;
            }
            /// scroll tree view to show top-most inserted element
            fileView->scrollTo(minRowTargetModelIndex, QAbstractItemView::PositionAtTop);

            /// clean up
            delete file;
            /// return true if at least one element was inserted
            return startRow <= endRow;
        } else if (!element.isNull()) {
            /// Regular text seems to have been dropped on a specific element

            QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
            if (!entry.isNull()) {
                qDebug() << "Adding in entry" << entry->id();

                /// Check if text looks like an URL
                const QUrl url(text);
                if (url.isValid()) {
                    return AssociatedFilesUI::associateUrl(url, entry, fileView->fileModel()->bibliographyFile(), fileView);
                }
            }
        } else
            qDebug() << "Don't know what to do with " << text;

        return false;
    }
};

Clipboard::Clipboard(FileView *fileView)
        : QObject(fileView), d(new ClipboardPrivate(fileView, this))
{
    connect(fileView, SIGNAL(editorMouseEvent(QMouseEvent*)), this, SLOT(editorMouseEvent(QMouseEvent*)));
    connect(fileView, SIGNAL(editorDragEnterEvent(QDragEnterEvent*)), this, SLOT(editorDragEnterEvent(QDragEnterEvent*)));
    connect(fileView, SIGNAL(editorDragMoveEvent(QDragMoveEvent*)), this, SLOT(editorDragMoveEvent(QDragMoveEvent*)));
    connect(fileView, SIGNAL(editorDropEvent(QDropEvent*)), this, SLOT(editorDropEvent(QDropEvent*)));
    fileView->setAcceptDrops(!fileView->isReadOnly());
}

Clipboard::~Clipboard()
{
    delete d;
}

void Clipboard::cut()
{
    copy();
    d->fileView->selectionDelete();
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
    QModelIndexList mil = d->fileView->selectionModel()->selectedRows();
    for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
        QSharedPointer<Entry> entry = d->fileView->fileModel()->element(d->fileView->sortFilterProxyModel()->mapToSource(*it).row()).dynamicCast<Entry>();
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
    const bool modified = d->insertText(clipboard->text(), d->fileView->currentElement());
    if (modified)
        d->fileView->externalModification();
}


void Clipboard::editorMouseEvent(QMouseEvent *event)
{
    if (!(event->buttons()&Qt::LeftButton))
        return;

    if (d->previousPosition.x() > -1 && (event->pos() - d->previousPosition).manhattanLength() >= QApplication::startDragDistance()) {
        QString text = d->selectionToText();

        QDrag *drag = new QDrag(d->fileView);
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
    if (event->mimeData()->hasText() && !d->fileView->isReadOnly())
        event->acceptProposedAction();
}

void Clipboard::editorDragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasText() && !d->fileView->isReadOnly())
        event->acceptProposedAction();
}

void Clipboard::editorDropEvent(QDropEvent *event)
{
    QString text = event->mimeData()->text();

    if (!text.isEmpty() && !d->fileView->isReadOnly()) {
        QSharedPointer<Element> element;
        /// Locate element drop was performed on (if dropped, and not some copy&paste)
        QModelIndex dropIndex = d->fileView->indexAt(event->pos());
        if (dropIndex.isValid())
            element = d->fileView->elementAt(dropIndex);
        if (element.isNull()) {
            /// Still invalid element? Use current one
            element = d->fileView->currentElement();
        }

        const bool modified = d->insertText(text, element);
        if (modified)
            d->fileView->externalModification();
    }
}
