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

#include "associatedfiles.h"

#include <QFileInfo>
#include <QDir>

#include <KIO/CopyJob>
#include <KJobWidgets>

#include <Preferences>
#include "logging_networking.h"

QString AssociatedFiles::relativeFilename(const QUrl &documentUrl, const QUrl &baseUrl) {
    if (!documentUrl.isValid()) {
        /// Document URL has to point to a file location or URL but is invalid
        return QString();
    }
    if (!baseUrl.isValid() || baseUrl.isRelative()) {
        /// Base URL has to point to an absolute file location or URL and must be valid
        return documentUrl.url(QUrl::PreferLocalFile);
    }
    if (documentUrl.scheme() != baseUrl.scheme() || (documentUrl.scheme() != QStringLiteral("file") && documentUrl.host() != baseUrl.host())) {
        /// Document URL and base URL do not match (protocol, host, ...)
        return documentUrl.url(QUrl::PreferLocalFile);
    }

    /// First, resolve the provided document URL to an absolute URL
    /// using the given base URL
    QUrl internaldocumentUrl = documentUrl;
    if (internaldocumentUrl.isRelative())
        internaldocumentUrl = baseUrl.resolved(internaldocumentUrl);

    /// Get the absolute path of the base URL
    const QString baseUrlDirectory = QFileInfo(baseUrl.path()).absolutePath();

    /// Let QDir calculate the relative directory
    return QDir(baseUrlDirectory).relativeFilePath(internaldocumentUrl.path());
}

QString AssociatedFiles::absoluteFilename(const QUrl &documentUrl, const QUrl &baseUrl) {
    if (!documentUrl.isValid()) {
        /// Document URL has to point to a file location or URL but is invalid
        return QString();
    }
    if (documentUrl.isRelative() && (!baseUrl.isValid() || baseUrl.isRelative())) {
        /// Base URL has to point to an absolute, valid file location or URL if the document URL is relative
        return documentUrl.url(QUrl::PreferLocalFile);
    }
    if (!documentUrl.isRelative() && (documentUrl.scheme() != baseUrl.scheme() || (documentUrl.scheme() != QStringLiteral("file") && documentUrl.host() != baseUrl.host()))) {
        /// Document URL and base URL do not match (protocol, host, ...), but necessary if the document URL is relative
        return documentUrl.url(QUrl::PreferLocalFile);
    }

    /// Resolve the provided document URL to an absolute URL
    /// using the given base URL
    QUrl internaldocumentUrl = documentUrl;
    if (internaldocumentUrl.isRelative())
        internaldocumentUrl = baseUrl.resolved(internaldocumentUrl);

    return internaldocumentUrl.url(QUrl::PreferLocalFile);
}

QString AssociatedFiles::insertUrl(const QUrl &documentUrl, QSharedPointer<Entry> &entry, const File *bibTeXFile, PathType pathType) {
    const QString finalUrl = computeAssociateString(documentUrl, bibTeXFile, pathType);

    bool alreadyContained = false;
    for (QMap<QString, Value>::ConstIterator it = entry->constBegin(); !alreadyContained && it != entry->constEnd(); ++it) {
        const Value v = it.value();
        for (Value::ConstIterator vit = v.constBegin(); !alreadyContained && vit != v.constEnd(); ++vit) {
            if (PlainTextValue::text(*vit) == finalUrl)
                alreadyContained = true;
        }
    }
    if (!alreadyContained) {
        const QString field = documentUrl.isLocalFile() ? (Preferences::instance().bibliographySystem() == Preferences::BibliographySystem::BibTeX ? Entry::ftLocalFile : Entry::ftFile) : Entry::ftUrl;
        Value value = entry->value(field);
        value.append(QSharedPointer<VerbatimText>(new VerbatimText(finalUrl)));
        entry->insert(field, value);
    }

    return finalUrl;
}

QString AssociatedFiles::computeAssociateString(const QUrl &documentUrl, const File *bibTeXFile, PathType pathType) {
    const QUrl baseUrl = bibTeXFile != nullptr && bibTeXFile->hasProperty(File::Url) ? bibTeXFile->property(File::Url).toUrl() : QUrl();
    return pathType == PathType::Absolute ? absoluteFilename(documentUrl, baseUrl) : relativeFilename(documentUrl, baseUrl);
}

QPair<QUrl, QUrl> AssociatedFiles::computeSourceDestinationUrls(const QUrl &sourceUrl, const QString &entryId, const File *bibTeXFile, RenameOperation renameOperation, const QString &userDefinedFilename)
{
    if (entryId.isEmpty() && renameOperation == RenameOperation::EntryId) {
        /// If no entry id was given but still a rename after entry id was requested,
        /// revert choice and enforce keeping the original name
        renameOperation = RenameOperation::KeepName;
    }

    const QUrl bibTeXFileUrl = bibTeXFile != nullptr && bibTeXFile->hasProperty(File::Url) ? bibTeXFile->property(File::Url).toUrl() : QUrl();
    if (!bibTeXFileUrl.isValid()) return QPair<QUrl, QUrl>();

    /// If URL of document to be associated is relative and current file has an URL, resolve relative URL
    const QUrl absoluteSourceUrl = sourceUrl.isRelative() ? bibTeXFileUrl.resolved(sourceUrl) : sourceUrl;

    /// Compute the filename for the move/copy operation's target
    const QFileInfo internalSourceInfo(absoluteSourceUrl.path());
    QString filename = internalSourceInfo.fileName();
    QString suffix = internalSourceInfo.suffix();
    if (suffix.isEmpty())
        suffix = QStringLiteral("html");
    if (filename.isEmpty() || renameOperation == RenameOperation::EntryId)
        filename = entryId + QLatin1Char('.') + suffix;
    if (filename.isEmpty() || renameOperation == RenameOperation::UserDefined)
        filename = userDefinedFilename;

    /// Assemble the target URL from the bibliography's URL and the filename as computed above
    QUrl targetUrl = bibTeXFileUrl;
    const QString targetPath = QFileInfo(targetUrl.path()).absolutePath();
    targetUrl.setPath(targetPath + QDir::separator() + filename);

    return QPair<QUrl, QUrl>(absoluteSourceUrl, targetUrl);
}

QUrl AssociatedFiles::copyDocument(const QUrl &sourceUrl, const QString &entryId, const File *bibTeXFile, RenameOperation renameOperation, MoveCopyOperation moveCopyOperation, QWidget *widget, const QString &userDefinedFilename)
{
    if (moveCopyOperation == MoveCopyOperation::None)
        /// If nothing to be copied or moved, the target URL is equal to the source URL
        return sourceUrl;

    const QPair<QUrl, QUrl> r = computeSourceDestinationUrls(sourceUrl, entryId, bibTeXFile, renameOperation, userDefinedFilename);
    const QUrl internalSourceUrl = r.first, targetUrl = r.second;

    bool success = true;
    if (internalSourceUrl.isLocalFile() && targetUrl.isLocalFile()) {
        QFile(targetUrl.path()).remove();
        success &= QFile::copy(internalSourceUrl.path(), targetUrl.path());
        if (success && moveCopyOperation == MoveCopyOperation::Move) {
            success &= QFile(internalSourceUrl.path()).remove();
        }
    } else if (internalSourceUrl.isValid() && targetUrl.isValid()) {
        // FIXME non-blocking
        KIO::CopyJob *moveCopyJob = moveCopyOperation == MoveCopyOperation::Move ? KIO::move(sourceUrl, targetUrl, KIO::HideProgressInfo | KIO::Overwrite) : KIO::copy(sourceUrl, targetUrl, KIO::HideProgressInfo | KIO::Overwrite);
        KJobWidgets::setWindow(moveCopyJob, widget);
        success &= moveCopyJob->exec();
    } else {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Either sourceUrl or targetUrl is invalid";
        return QUrl();
    }

    if (!success) return QUrl(); ///< either copy/move or delete operation failed

    return targetUrl;
}
