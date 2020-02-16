/***************************************************************************
 *   Copyright (C) 2004-2020 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <Preferences>
#include <models/FileModel>
#include <FileImporterBibTeX>
#include <FileExporterBibTeX>
#include <File>
#include "fileview.h"
#include "element/associatedfilesui.h"
#include "logging_gui.h"

class Clipboard::ClipboardPrivate
{
public:
    FileView *fileView;
    QPoint previousPosition;

    ClipboardPrivate(FileView *fv, Clipboard *parent)
            : fileView(fv) {
        Q_UNUSED(parent)
    }

    QString selectionToText() {
        FileModel *model = fileView->fileModel();
        if (model == nullptr) return QString();

        const QModelIndexList mil = fileView->selectionModel()->selectedRows();
        QScopedPointer<File> file(new File());
        for (const QModelIndex &index : mil)
            file->append(model->element(fileView->sortFilterProxyModel()->mapToSource(index).row()));

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

    bool insertUrl(const QString &text, QSharedPointer<Entry> entry) {
        const QUrl url = QUrl::fromUserInput(text);
        return insertUrl(url, entry);
    }

    /**
     * Makes an attempt to insert the passed text as an URL to the given
     * element. May fail for various reasons, such as the text not being
     * a valid URL or the element being invalid.
     */
    bool insertUrl(const QUrl &url, QSharedPointer<Entry> entry) {
        if (entry.isNull()) return false;
        if (!url.isValid()) return false;
        FileModel *model = fileView->fileModel();
        if (model == nullptr) return false;

        return !AssociatedFilesUI::associateUrl(url, entry, model->bibliographyFile(), true, fileView).isEmpty();
    }

    /**
     * Given a fragment of BibTeX code, insert the elements contained in
     * this code into the current file
     * @param code BibTeX code in text form
     * @return true if at least one element got inserted and no error occurred
     */
    bool insertBibTeX(const QString &code) {
        /// Use BibTeX importer to generate representation from plain text
        FileImporterBibTeX importer(fileView);
        QScopedPointer<File> file(importer.fromString(code));
        if (!file.isNull() && !file->isEmpty()) {
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

            /// Return true if at least one element was inserted
            if (startRow <= endRow)
                return true;
        }

        return false;
    }

    bool insertText(const QString &text, QSharedPointer<Element> element = QSharedPointer<Element>()) {
        /// Cast current element into an entry which then may be used in case an URL needs to be inserted into it
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        const QRegularExpressionMatch urlRegExpMatch = KBibTeX::urlRegExp.match(text);
        /// Quick check if passed text's beginning looks like an URL;
        /// in this case try to add it to the current/selected entry
        if (urlRegExpMatch.hasMatch() && urlRegExpMatch.capturedStart() == 0 && !entry.isNull()) {
            if (insertUrl(urlRegExpMatch.captured(0), entry))
                return true;
        }

        /// Assumption: user dropped a piece of BibTeX code,
        if (insertBibTeX(text))
            return true;

        qCInfo(LOG_KBIBTEX_GUI) << "This text cannot be in inserted, looks neither like a URL nor like BibTeX code: " << text;

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
    FileModel *model = d->fileView != nullptr ? d->fileView->fileModel() : nullptr;
    if (model == nullptr) return;

    QStringList references;
    const QModelIndexList mil = d->fileView->selectionModel()->selectedRows();
    references.reserve(mil.size());
    for (const QModelIndex &index : mil) {
        QSharedPointer<Entry> entry = model->element(d->fileView->sortFilterProxyModel()->mapToSource(index).row()).dynamicCast<Entry>();
        if (!entry.isNull())
            references << entry->id();
    }

    if (!references.isEmpty()) {
        QClipboard *clipboard = QApplication::clipboard();
        QString text = references.join(QStringLiteral(","));

        const QString copyReferenceCommand = Preferences::instance().copyReferenceCommand();
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

        /// Cast current element into an entry which then may be used in case an URL needs to be inserted into it
        QSharedPointer<Entry> entry = element.isNull() ? QSharedPointer<Entry>() : element.dynamicCast<Entry>();

        const bool modified = !urls.isEmpty() && !entry.isNull() ? d->insertUrl(urls.first(), entry) : (!text.isEmpty() ? d->insertText(text, element) : false);
        if (modified)
            d->fileView->externalModification();
    }
}
