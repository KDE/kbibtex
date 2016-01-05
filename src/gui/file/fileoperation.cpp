/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                      2015 by Shunsuke Shimizu <grafi@grafi.jp>          *
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

#include "fileoperation.h"

#include <QApplication>
#include <QMimeData>
#include <QBuffer>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KDebug>

#include "fileview.h"
#include "filemodel.h"
#include "fileimporterbibtex.h"
#include "fileimporterris.h"
#include "fileimporterbibutils.h"
#include "fileexporterbibtex.h"
#include "file.h"
#include "associatedfilesui.h"
#include "clipboard.h"

/**
 * @author Shunsuke Shimizu <grafi@grafi.jp>
 */
class FileOperation::FileOperationPrivate
{
private:
    FileOperation *parent;

public:
    FileView *fileView;
    KSharedConfigPtr config;
    const QString configGroupName;

    FileOperationPrivate(FileView *fv, FileOperation *p)
            : parent(p), fileView(fv), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("General")) {
        /// nothing
    }

    FileModel *fileModel()
    {
        return fileView->fileModel();
    }

    QList<int> insertEntries(const QString &text, FileImporter &importer)
    {
        QList<int> result;
        if (parent->checkReadOnly()) {
            kWarning() << "Cannot insert entries in read-only mode";
            return result;
        }

        File *file = importer.fromString(text);
        if (file == NULL) {
            kWarning() << "Text " << text << " could not be read by importer";
            return result;
        }
        if (file->isEmpty()) {
            kWarning() << "Text " << text << " gave no entries";
            delete file;
            return result;
        }

        FileModel *fm = fileModel();
        QSortFilterProxyModel *sfpModel = fileView->sortFilterProxyModel();

        /// Insert new elements one by one
        const int startRow = fm->rowCount(); ///< Memorize row where insertion started
        for (File::Iterator it = file->begin(); it != file->end(); ++it)
            fm->insertRow(*it, fileView->model()->rowCount());
        const int endRow = fm->rowCount() - 1; ///< Memorize row where insertion ended

        /// Select newly inserted elements
        QItemSelectionModel *ism = fileView->selectionModel();
        ism->clear();
        /// Keep track of the insert element which is most upwards in the list when inserted
        QModelIndex minRowTargetModelIndex;
        /// Highlight those rows in the editor which correspond to newly inserted elements
        for (int i = startRow; i <= endRow; ++i) {
            QModelIndex targetModelIndex = sfpModel->mapFromSource(fm->index(i, 0));
            ism->select(targetModelIndex, QItemSelectionModel::Rows | QItemSelectionModel::Select);

            /// Update the most upward inserted element
            if (!minRowTargetModelIndex.isValid() || minRowTargetModelIndex.row() > targetModelIndex.row())
                minRowTargetModelIndex = targetModelIndex;

            // construct the result
            result.push_back(i);
        }
        /// Scroll tree view to show top-most inserted element
        fileView->scrollTo(minRowTargetModelIndex, QAbstractItemView::PositionAtTop);

        /// Clean up
        delete file;
        /// Return true if at least one element was inserted

        if (!result.isEmpty())
            fileView->externalModification();
        else
            kDebug() << "Failed to insert bibliographic elements into open file";
        return result;
    }

};

FileOperation::FileOperation(FileView *fileView)
        : QObject(fileView), d(new FileOperationPrivate(fileView, this))
{
    /// nothing
}

FileOperation::~FileOperation()
{
    delete d;
}


QList<int> FileOperation::selectedEntryIndexes()
{
    QList<int> result;
    const QModelIndexList mil = d->fileView->selectionModel()->selectedRows();
    for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
        result.push_back(d->fileView->sortFilterProxyModel()->mapToSource(*it).row());
    }
    return result;
}

QString FileOperation::entryIndexesToText(const QList<int> &entryIndexes)
{
    File *file = new File();
    for (QList<int>::ConstIterator it = entryIndexes.constBegin(); it != entryIndexes.constEnd(); ++it)
        file->append(d->fileModel()->element(*it));

    FileExporterBibTeX exporter;
    exporter.setEncoding(QLatin1String("latex"));
    QBuffer buffer(d->fileView);
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

QString FileOperation::entryIndexesToReferences(const QList<int> &entryIndexes)
{
    QStringList references;
    for (QList<int>::ConstIterator it = entryIndexes.constBegin(); it != entryIndexes.constEnd(); ++it) {
        QSharedPointer<Entry> entry = d->fileModel()->element(*it).dynamicCast<Entry>();
        if (!entry.isNull())
            references << entry->id();
    }

    QString text;
    if (!references.isEmpty()) {
        text = references.join(",");

        KConfigGroup configGroup(d->config, d->configGroupName);
        const QString copyReferenceCommand = configGroup.readEntry(Clipboard::keyCopyReferenceCommand, Clipboard::defaultCopyReferenceCommand);
        if (!copyReferenceCommand.isEmpty())
            text = QString(QLatin1String("\\%1{%2}")).arg(copyReferenceCommand).arg(text);
    }

    return text;
}

/**
 * Makes an attempt to insert the passed text as an URL to the given
 * element. May fail for various reasons, such as the text not being
 * a valid URL or the element being invalid.
 */
bool FileOperation::insertUrl(const QString &text, int entryIndex)
{
    if (checkReadOnly())
        return false;

    QSharedPointer<Entry> entry = d->fileModel()->element(entryIndex).dynamicCast<Entry>();
    if (entry.isNull()) return false;
    const QUrl url = QUrl::fromUserInput(text);
    if (!url.isValid()) return false;

    kDebug() << "Adding URL " << url.toString() << "to entry" << entry->id();
    bool success = AssociatedFilesUI::associateUrl(url, entry, d->fileModel()->bibliographyFile(), d->fileView);
    if (success)
        d->fileView->externalModification();
    else
        kDebug() << "Adding URL " << url.toString() << " to entry" << entry->id() << " failed";
    return success;
}

QList<int> FileOperation::insertEntries(const QString &text, const QString &mimeType)
{
    if (mimeType == QLatin1String("application/x-bibtex") || mimeType == QLatin1String("text/x-bibtex")) {
        FileImporterBibTeX importerBibTeX;
        return d->insertEntries(text, importerBibTeX);
    } else if (mimeType == QLatin1String("application/x-research-info-systems") || mimeType == QLatin1String("text/x-research-info-systems")) {
        FileImporterRIS importerRIS;
        return d->insertEntries(text, importerRIS);
    } else if (FileImporterBibUtils::available() && mimeType == QLatin1String("application/x-isi-export-format")) {
        FileImporterBibUtils importerBibUtils;
        importerBibUtils.setFormat(BibUtils::ISI);
        return d->insertEntries(text, importerBibUtils);
    } else if (FileImporterBibUtils::available() && (mimeType == QLatin1String("application/x-endnote-refer") || mimeType == QLatin1String("text/x-endnote-refer") || mimeType == QLatin1String("application/x-endnote-library"))) {
        FileImporterBibUtils importerBibUtils;
        importerBibUtils.setFormat(BibUtils::EndNote);
        return d->insertEntries(text, importerBibUtils);
    } else {
        kDebug() << "Unsupported mimeType:" << mimeType;
        return QList<int>();
    }
}

/**
 * Assumption: user dropped a piece of BibTeX code,
 * use BibTeX importer to generate representation from plain text.
 */


bool FileOperation::checkReadOnly()
{
    if (d->fileView->isReadOnly()) {
        kDebug() << "Attempting to modify readonly bib";
        return true;
    } else {
        return false;
    }
}

FileView *FileOperation::fileView()
{
    return d->fileView;
}
