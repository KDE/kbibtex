/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QMimeType>

#include <Preferences>
#include <models/FileModel>
#include <FileImporterBibTeX>
#include <FileExporterBibTeX>
#include <File>
#include <FileInfo>
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

    bool insertText(const QString &text, QSharedPointer<Element> element) {
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

    static QSet<QUrl> urlsInMimeData(const QMimeData *mimeData, const QStringList &acceptableMimeTypes) {
        QSet<QUrl> result;
        if (mimeData->hasUrls()) {
            const QList<QUrl> urls = mimeData->urls();
            for (const QUrl &url : urls) {
                if (url.isValid()) {
                    if (acceptableMimeTypes.isEmpty())
                        /// No limitations regarding which mime types are allowed?
                        /// Then accept any URL
                        result.insert(url);
                    const QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
                    if (acceptableMimeTypes.contains(mimeType.name())) {
                        /// So, there is a list of acceptable mime types.
                        /// Only if URL points to a file that matches a mime type
                        /// from list return URL
                        result.insert(url);
                    }
                }
            }
        } else if (mimeData->hasText()) {
            const QString text = QString::fromUtf8(mimeData->data(QStringLiteral("text/plain")));
            QRegularExpressionMatchIterator urlRegExpMatchIt = KBibTeX::urlRegExp.globalMatch(text);
            while (urlRegExpMatchIt.hasNext()) {
                const QRegularExpressionMatch urlRegExpMatch = urlRegExpMatchIt.next();
                const QUrl url = QUrl::fromUserInput(urlRegExpMatch.captured());
                if (url.isValid()) {
                    if (acceptableMimeTypes.isEmpty())
                        /// No limitations regarding which mime types are allowed?
                        /// Then accept any URL
                        result.insert(url);
                    const QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
                    if (acceptableMimeTypes.contains(mimeType.name())) {
                        /// So, there is a list of acceptable mime types.
                        /// Only if URL points to a file that matches a mime type
                        /// from list return URL
                        result.insert(url);
                    }
                }
            }
        }

        /// Return all found URLs (if any)
        return result;
    }

    static QSet<QUrl> urlsToOpen(const QMimeData *mimeData) {
        static const QStringList mimeTypesThatCanBeOpened{QStringLiteral("text/x-bibtex")}; // TODO share list of mime types with file-open dialogs?
        return urlsInMimeData(mimeData, mimeTypesThatCanBeOpened);
    }

    static QSet<QUrl> urlsToAssociate(const QMimeData *mimeData) {
        /// UNUSED static const QStringList mimeTypesThatCanBeAssociated{QStringLiteral("application/pdf")}; // TODO more mime types
        return urlsInMimeData(mimeData, QStringList() /** accepting any MIME type */);
    }

    QSharedPointer<Entry> dropTarget(const QPoint &pos) const {
        /// Locate element drop was performed on
        const QModelIndex dropIndex = fileView->indexAt(pos);
        if (dropIndex.isValid())
            return fileView->elementAt(dropIndex).dynamicCast<Entry>();
        return QSharedPointer<Entry>();
    }

    enum LooksLike {LooksLikeUnknown = 0, LooksLikeBibTeX, LooksLikeURL};

    LooksLike looksLikeWhat(const QMimeData *mimeData) const {
        if (mimeData->hasText()) {
            QString text = QString::fromUtf8(mimeData->data(QStringLiteral("text/plain")));
            const int p1 = text.indexOf(QLatin1Char('@'));
            const int p2 = text.lastIndexOf(QLatin1Char('}'));
            const int p3 = text.lastIndexOf(QLatin1Char(')'));
            if (p1 >= 0 && (p2 >= 0 || p3 >= 0)) {
                text = text.mid(p1, qMax(p2, p3) - p1 + 1);
                static const QRegularExpression bibTeXElement(QStringLiteral("^@([a-z]{5,})[{()]"), QRegularExpression::CaseInsensitiveOption);
                const QRegularExpressionMatch bibTeXElementMatch = bibTeXElement.match(text.left(64));
                if (bibTeXElementMatch.hasMatch() && bibTeXElementMatch.captured(1) != QStringLiteral("import") /** ignore '@import' */)
                    return LooksLikeBibTeX;
            }
            // TODO more tests for more bibliography formats
            else {
                const QRegularExpressionMatch urlRegExpMatch = KBibTeX::urlRegExp.match(text.left(256));
                if (urlRegExpMatch.hasMatch() && urlRegExpMatch.capturedStart() == 0)
                    return LooksLikeURL;
            }
        } else if (mimeData->hasUrls() && !mimeData->urls().isEmpty())
            return LooksLikeURL;

        return LooksLikeUnknown;
    }

    bool acceptableDropAction(const QMimeData *mimeData, const QPoint &pos) {
        if (!urlsToOpen(mimeData).isEmpty())
            /// Data to be dropped is an URL that should be opened,
            /// which is a job for the shell, but not for this part
            /// where this Clipboard belongs to. By ignoring this event,
            /// it will be delegated to the underlying shell, similarly
            /// as if the URL would be dropped on a KBibTeX program
            /// window with no file open.
            return false;
        else if (looksLikeWhat(mimeData) != LooksLikeUnknown || (!dropTarget(pos).isNull() && !urlsToAssociate(mimeData).isEmpty()))
            /// The dropped data is either text in a known bibliography
            /// format or the drop happens on an entry and the dropped
            /// data is an URL that can be associated with this entry
            /// (e.g. an URL to a PDF document).
            return true;
        else
            return false;
    }
};

Clipboard::Clipboard(FileView *fileView)
        : QObject(fileView), d(new ClipboardPrivate(fileView, this))
{
    fileView->setClipboard(this);
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
        QByteArray data = text.toUtf8();
        mimeData->setData(QStringLiteral("text/plain"), data);
        drag->setMimeData(mimeData);

        drag->exec(Qt::CopyAction);
    }

    d->previousPosition = event->pos();
}

void Clipboard::editorDragEnterEvent(QDragEnterEvent *event)
{
    if (d->fileView->isReadOnly())
        event->ignore();
    else if (d->acceptableDropAction(event->mimeData(), event->pos()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void Clipboard::editorDragMoveEvent(QDragMoveEvent *event)
{
    if (d->fileView->isReadOnly())
        event->ignore();
    else if (d->acceptableDropAction(event->mimeData(), event->pos()))
        event->acceptProposedAction();
    else
        event->ignore();
}

void Clipboard::editorDropEvent(QDropEvent *event)
{
    if (d->fileView->isReadOnly()) {
        event->ignore();
        return;
    }

    bool modified = false;
    const ClipboardPrivate::LooksLike looksLike = d->looksLikeWhat(event->mimeData());
    if (looksLike == ClipboardPrivate::LooksLikeBibTeX) {
        /// The dropped data looks like BibTeX code
        modified = d->insertBibTeX(event->mimeData()->text());
    } else if (looksLike == ClipboardPrivate::LooksLikeURL) {
        /// Dropped data does not look like a known bibliography
        /// format.
        /// Check if dropped data looks like an URL (e.g. pointing
        /// to a PDF document) and if the drop happens onto an
        /// bibliography entry (and not a comment or an empty area
        /// in the list view, for example)
        const QSharedPointer<Entry> dropTarget = d->dropTarget(event->pos());
        if (!dropTarget.isNull()) {
            const QSet<QUrl> urls = d->urlsToAssociate(event->mimeData());
            for (const QUrl &urlToAssociate : urls)
                modified |= d->insertUrl(urlToAssociate, dropTarget);
        }
    }

    if (modified)
        d->fileView->externalModification();
}

QSet<QUrl> Clipboard::urlsToOpen(const QMimeData *mimeData)
{
    return ClipboardPrivate::urlsToOpen(mimeData);
}
