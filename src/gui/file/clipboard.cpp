/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "clipboard.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QMouseEvent>
#include <QDrag>

#include <KConfigGroup>
#include <KSharedConfig>

#include "fileview.h"
#include "models/filemodel.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"
#include "associatedfilesui.h"
#include "logging_gui.h"

const QString Clipboard::keyCopyReferenceCommand = QStringLiteral("copyReferenceCommand");
const QString Clipboard::defaultCopyReferenceCommand = QStringLiteral("");

class Clipboard::ClipboardPrivate
{
public:
    FileView *fileView;
    QPoint previousPosition;
    KSharedConfigPtr config;
    const QString configGroupName;

    ClipboardPrivate(FileView *fv, Clipboard *parent)
            : fileView(fv), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("General")) {
        Q_UNUSED(parent)
    }

    QString selectionToText() {
        const QModelIndexList mil = fileView->selectionModel()->selectedRows();
        QScopedPointer<File> file(new File());
        for (const QModelIndex &index : mil)
            file->append(fileView->fileModel()->element(fileView->sortFilterProxyModel()->mapToSource(index).row()));

        FileExporterBibTeX exporter(fileView);
        exporter.setEncoding(QStringLiteral("latex"));
        QBuffer buffer(fileView);
        buffer.open(QBuffer::WriteOnly);
        const bool success = exporter.save(&buffer, file.data());
        buffer.close();
        if (!success)
            return QString();

        buffer.open(QBuffer::ReadOnly);
        const QString text = QString::fromUtf8(buffer.readAll());
        buffer.close();

        return text;
    }

    /**
     * Makes an attempt to insert the passed text as an URL to the given
     * element. May fail for various reasons, such as the text not being
     * a valid URL or the element being invalid.
     */
    bool insertUrl(const QString &text, QSharedPointer<Element> element = QSharedPointer<Element>()) {
        const QUrl url = QUrl::fromUserInput(text);
        return insertUrl(url, element);
    }

    bool insertUrl(const QUrl &url, QSharedPointer<Element> element = QSharedPointer<Element>()) {
        if (element.isNull()) return false;
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        if (entry.isNull()) return false;
        if (!url.isValid()) return false;

        qCDebug(LOG_KBIBTEX_GUI) << "About to add URL " << url.toDisplayString() << " to entry" << entry->id();
        return AssociatedFilesUI::associateUrl(url, entry, fileView->fileModel()->bibliographyFile(), fileView);
    }

    bool insertText(const QString &text, QSharedPointer<Element> element = QSharedPointer<Element>()) {
        bool insertUrlPreviouslyCalled = false;

        if (text.startsWith(QStringLiteral("http://")) || text.startsWith(QStringLiteral("https://")) || text.startsWith(QStringLiteral("file://"))) {
            /// Quick check if passed text looks like an URL;
            /// in this case try to add it to the current/selected entry
            if (insertUrl(text, element))
                return true;
            else
                insertUrlPreviouslyCalled = true;
        }

        /// Assumption: user dropped a piece of BibTeX code,
        /// use BibTeX importer to generate representation from plain text
        FileImporterBibTeX importer(fileView);
        File *file = importer.fromString(text);
        if (file != nullptr) {
            if (!file->isEmpty()) {
                FileModel *fileModel = fileView->fileModel();
                QSortFilterProxyModel *sfpModel = fileView->sortFilterProxyModel();

                /// Insert new elements one by one
                const int startRow = fileModel->rowCount(); ///< Memorize row where insertion started
                for (const auto &element : const_cast<const File &>(*file))
                    fileModel->insertRow(element, fileView->model()->rowCount());
                const int endRow = fileModel->rowCount() - 1; ///< Memorize row where insertion ended

                /// Select newly inserted elements
                QItemSelectionModel *ism = fileView->selectionModel();
                ism->clear();
                /// Keep track of the insert element which is most upwards in the list when inserted
                QModelIndex minRowTargetModelIndex;
                /// Highlight those rows in the editor which correspond to newly inserted elements
                for (int i = startRow; i <= endRow; ++i) {
                    QModelIndex targetModelIndex = sfpModel->mapFromSource(fileModel->index(i, 0));
                    ism->select(targetModelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);

                    /// Update the most upward inserted element
                    if (!minRowTargetModelIndex.isValid() || minRowTargetModelIndex.row() > targetModelIndex.row())
                        minRowTargetModelIndex = targetModelIndex;
                }
                /// Scroll tree view to show top-most inserted element
                fileView->scrollTo(minRowTargetModelIndex, QAbstractItemView::PositionAtTop);

                /// Clean up
                delete file;
                /// Return true if at least one element was inserted
                if (startRow <= endRow)
                    return true;
            } else
                delete file; ///< clean up
        }

        if (!insertUrlPreviouslyCalled) {
            /// If not tried above (e.g. if another protocol as the
            /// ones listed above is passed), call insertUrl(..) now
            if (insertUrl(text, element))
                return true;
        }

        qCDebug(LOG_KBIBTEX_GUI) << "Don't know what to do with " << text;

        return false;
    }
};

Clipboard::Clipboard(FileView *fileView)
        : QObject(fileView), d(new ClipboardPrivate(fileView, this))
{
    connect(fileView, &FileView::editorMouseEvent, this, &Clipboard::editorMouseEvent);
    connect(fileView, &FileView::editorDragEnterEvent, this, &Clipboard::editorDragEnterEvent);
    connect(fileView, &FileView::editorDragMoveEvent, this, &Clipboard::editorDragMoveEvent);
    connect(fileView, &FileView::editorDropEvent, this, &Clipboard::editorDropEvent);
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
    const QModelIndexList mil = d->fileView->selectionModel()->selectedRows();
    references.reserve(mil.size());
    for (const QModelIndex &index : mil) {
        QSharedPointer<Entry> entry = d->fileView->fileModel()->element(d->fileView->sortFilterProxyModel()->mapToSource(index).row()).dynamicCast<Entry>();
        if (!entry.isNull())
            references << entry->id();
    }

    if (!references.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        QString text = references.join(QStringLiteral(","));

        KConfigGroup configGroup(d->config, d->configGroupName);
        const QString copyReferenceCommand = configGroup.readEntry(keyCopyReferenceCommand, defaultCopyReferenceCommand);
        if (!copyReferenceCommand.isEmpty())
            text = QString(QStringLiteral("\\%1{%2}")).arg(copyReferenceCommand, text);

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
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if (d->previousPosition.x() > -1 && (event->pos() - d->previousPosition).manhattanLength() >= QApplication::startDragDistance()) {
        QString text = d->selectionToText();

        QDrag *drag = new QDrag(d->fileView);
        QMimeData *mimeData = new QMimeData();
        QByteArray data = text.toLatin1();
        mimeData->setData(QStringLiteral("text/plain"), data);
        drag->setMimeData(mimeData);

        drag->exec(Qt::CopyAction);
    }

    d->previousPosition = event->pos();
}

void Clipboard::editorDragEnterEvent(QDragEnterEvent *event)
{
    if (d->fileView->isReadOnly()) {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasText() || event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void Clipboard::editorDragMoveEvent(QDragMoveEvent *event)
{
    if (d->fileView->isReadOnly()) {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasText() || event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void Clipboard::editorDropEvent(QDropEvent *event)
{
    if (d->fileView->isReadOnly()) {
        event->ignore();
        return;
    }

    if (event->mimeData()->hasText() || event->mimeData()->hasUrls()) {
        const QString text = event->mimeData()->text();
        const QList<QUrl> urls = event->mimeData()->urls();

        QSharedPointer<Element> element;
        /// Locate element drop was performed on (if dropped, and not some copy&paste)
        QModelIndex dropIndex = d->fileView->indexAt(event->pos());
        if (dropIndex.isValid())
            element = d->fileView->elementAt(dropIndex);
        if (element.isNull()) {
            /// Still invalid element? Use current one
            element = d->fileView->currentElement();
        }

        const bool modified = !urls.isEmpty() ? d->insertUrl(urls.first(), element) : (!text.isEmpty() ? d->insertText(text, element) : false);
        if (modified)
            d->fileView->externalModification();
    }
}
