/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

bool AssociatedFiles::urlIsLocal(const QUrl &url)
{
    // FIXME same function as in UrlListEdit; move to common code base?
    const QString scheme = url.scheme();
    /// Test various schemes such as "http", "https", "ftp", ...
    return !scheme.startsWith(QStringLiteral("http")) && !scheme.startsWith(QStringLiteral("ftp")) && !scheme.startsWith(QStringLiteral("sftp")) && !scheme.startsWith(QStringLiteral("fish")) && !scheme.startsWith(QStringLiteral("webdav")) && scheme != QStringLiteral("smb");
}

QString AssociatedFiles::relativeFilename(const QUrl &documentUrl, const QUrl &baseUrl) {
    if (documentUrl.isEmpty()) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "document URL has to point to a file location or URL";
        return documentUrl.url(QUrl::PreferLocalFile);
    }
    if (baseUrl.isEmpty() || baseUrl.isRelative()) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "base URL has to point to an absolute file location or URL";
        return documentUrl.url(QUrl::PreferLocalFile);
    }
    if (documentUrl.scheme() != baseUrl.scheme() || (documentUrl.scheme() != QStringLiteral("file") && documentUrl.host() != baseUrl.host())) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "document URL and base URL do not match (protocol, host, ...)";
        return documentUrl.url(QUrl::PreferLocalFile);
    }

    /// First, resolve the provided document URL to an absolute URL
    /// using the given base URL
    QUrl internalDocumentUrl = documentUrl;
    if (internalDocumentUrl.isRelative())
        internalDocumentUrl = baseUrl.resolved(internalDocumentUrl);

    /// Get the absolute path of the base URL
    const QString baseUrlDirectory = QFileInfo(baseUrl.path()).absolutePath();

    /// Let QDir calculate the relative directory
    return QDir(baseUrlDirectory).relativeFilePath(internalDocumentUrl.path());
}

QString AssociatedFiles::absoluteFilename(const QUrl &documentUrl, const QUrl &baseUrl) {
    if (documentUrl.isEmpty()) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "document URL has to point to a file location or URL";
        return documentUrl.url(QUrl::PreferLocalFile);
    }
    if (documentUrl.isRelative() && (baseUrl.isEmpty() || baseUrl.isRelative())) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "base URL has to point to an absolute file location or URL if the document URL is relative";
        return documentUrl.url(QUrl::PreferLocalFile);
    }
    if (documentUrl.isRelative() && (documentUrl.scheme() != baseUrl.scheme() || (documentUrl.scheme() != QStringLiteral("file") && documentUrl.host() != baseUrl.host()))) {
        qCWarning(LOG_KBIBTEX_NETWORKING) << "document URL and base URL do not match (protocol, host, ...), but necessary if the document URL is relative";
        return documentUrl.url(QUrl::PreferLocalFile);
    }

    /// Resolve the provided document URL to an absolute URL
    /// using the given base URL
    QUrl internalDocumentUrl = documentUrl;
    if (internalDocumentUrl.isRelative())
        internalDocumentUrl = baseUrl.resolved(internalDocumentUrl);

    return internalDocumentUrl.url(QUrl::PreferLocalFile);
}

QString AssociatedFiles::associateDocumentURL(const QUrl &document, QSharedPointer<Entry> &entry, const File *bibTeXFile, PathType pathType, const bool dryRun) {
    Q_ASSERT(bibTeXFile != nullptr); // FIXME more graceful?

    const QUrl baseUrl = bibTeXFile->property(File::Url).toUrl();
    if (baseUrl.isEmpty() && pathType == ptRelative) {
        /// If no base URL was given but still a relative path was requested,
        /// revert choice and enforce the generation of an absolute one
        pathType = ptAbsolute;
    }

    const bool isLocal = urlIsLocal(document);
    const QString field = isLocal ? (Preferences::instance().bibliographySystem() == Preferences::instance().BibTeX ? Entry::ftLocalFile : Entry::ftFile) : Entry::ftUrl;
    QString finalUrl = pathType == ptAbsolute ? absoluteFilename(document, baseUrl) : relativeFilename(document, baseUrl);

    if (!dryRun) { /// only if not pretending
        bool alreadyContained = false;
        for (QMap<QString, Value>::ConstIterator it = entry->constBegin(); !alreadyContained && it != entry->constEnd(); ++it) {
            const Value v = it.value();
            for (Value::ConstIterator vit = v.constBegin(); !alreadyContained && vit != v.constEnd(); ++vit) {
                if (PlainTextValue::text(*vit) == finalUrl)
                    alreadyContained = true;
            }
        }
        if (!alreadyContained) {
            Value value = entry->value(field);
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(finalUrl)));
            entry->insert(field, value);
        }
    }

    return finalUrl;
}

QString AssociatedFiles::associateDocumentURL(const QUrl &document, const File *bibTeXFile, PathType pathType) {
    Q_ASSERT(bibTeXFile != nullptr); // FIXME more graceful?

    const QUrl baseUrl = bibTeXFile->property(File::Url).toUrl();
    if (baseUrl.isEmpty() && pathType == ptRelative) {
        /// If no base URL was given but still a relative path was requested,
        /// revert choice and enforce the generation of an absolute one
        pathType = ptAbsolute;
    }

    QString finalUrl = pathType == ptAbsolute ? absoluteFilename(document, baseUrl) : relativeFilename(document, baseUrl);

    return finalUrl;
}

QUrl AssociatedFiles::copyDocument(const QUrl &sourceUrl, const QString &entryId, const File *bibTeXFile, RenameOperation renameOperation, MoveCopyOperation moveCopyOperation, QWidget *widget, const QString &userDefinedFilename, const bool dryRun) {
    Q_ASSERT(bibTeXFile != nullptr); // FIXME more graceful?

    if (moveCopyOperation == mcoNoCopyMove)
        return sourceUrl; /// nothing to do if no move or copy requested

    if (entryId.isEmpty() && renameOperation == roEntryId) {
        /// If no entry id was given but still a rename after entry id was requested,
        /// revert choice and enforce keeping the original name
        renameOperation = roKeepName;
    }

    const QUrl baseUrl = bibTeXFile->property(File::Url).toUrl();
    const QUrl internalSourceUrl = baseUrl.resolved(sourceUrl);

    const QFileInfo internalSourceInfo(internalSourceUrl.path());
    QString filename = internalSourceInfo.fileName();
    QString suffix = internalSourceInfo.suffix();
    if (suffix.isEmpty()) {
        suffix = QStringLiteral("html");
        filename.append(QLatin1Char('.')).append(suffix);
    }
    if (filename.isEmpty()) filename = internalSourceUrl.url(QUrl::PreferLocalFile).remove(QDir::separator()).remove(QLatin1Char('/')).remove(QLatin1Char(':')).remove(QLatin1Char('.')) + QStringLiteral(".") + suffix;

    if (!bibTeXFile->hasProperty(File::Url)) return QUrl(); /// no valid URL set of BibTeX file object
    QUrl targetUrl = bibTeXFile->property(File::Url).toUrl();
    if (targetUrl.isEmpty()) return QUrl(); /// no valid URL set of BibTeX file object
    const QString targetPath = QFileInfo(targetUrl.path()).absolutePath();
    targetUrl.setPath(targetPath + QDir::separator() + (renameOperation == roEntryId ? entryId + QStringLiteral(".") + suffix : (renameOperation == roUserDefined ? userDefinedFilename : filename)));

    if (!dryRun) { /// only if not pretending
        bool success = true;
        if (urlIsLocal(internalSourceUrl) && urlIsLocal(targetUrl)) {
            QFile(targetUrl.path()).remove();
            success &= QFile::copy(internalSourceUrl.path(), targetUrl.path()); // FIXME check if succeeded
            if (moveCopyOperation == mcoMove) {
                success &= QFile(internalSourceUrl.path()).remove();
            }
        } else {
            // FIXME non-blocking
            KIO::CopyJob *moveCopyJob = moveCopyOperation == mcoMove ? KIO::move(sourceUrl, targetUrl, KIO::HideProgressInfo | KIO::Overwrite) : KIO::copy(sourceUrl, targetUrl, KIO::HideProgressInfo | KIO::Overwrite);
            KJobWidgets::setWindow(moveCopyJob, widget);
            success &= moveCopyJob->exec();
        }

        if (!success) return QUrl(); ///< either copy/move or delete operation failed
    }

    return targetUrl;
}
