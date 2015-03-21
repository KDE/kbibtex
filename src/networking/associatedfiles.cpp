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

#include "associatedfiles.h"

#include <QFileInfo>
#include <QDir>

#include <KTemporaryFile>
#include <KIO/NetAccess>
#include <QDebug>

bool AssociatedFiles::urlIsLocal(const QUrl &url)
{
    // FIXME same function as in UrlListEdit; move to common code base?
    const QString scheme = url.scheme();
    /// Test various schemes such as "http", "https", "ftp", ...
    return !scheme.startsWith(QLatin1String("http")) && !scheme.startsWith(QLatin1String("ftp")) && !scheme.startsWith(QLatin1String("sftp")) && !scheme.startsWith(QLatin1String("fish")) && !scheme.startsWith(QLatin1String("webdav")) && scheme != QLatin1String("smb");
}

QString AssociatedFiles::relativeFilename(const QUrl &documentUrl, const QUrl &baseUrl) {
    if (documentUrl.isEmpty()) {
        qWarning() << "document URL has to point to a file location or URL";
        return documentUrl.pathOrUrl();
    }
    if (baseUrl.isEmpty() || baseUrl.isRelative()) {
        qWarning() << "base URL has to point to an absolute file location or URL";
        return documentUrl.pathOrUrl();
    }
    if (documentUrl.scheme() != baseUrl.scheme() || (documentUrl.scheme() != QLatin1String("file") && documentUrl.host() != baseUrl.host())) {
        qWarning() << "document URL and base URL do not match (protocol, host, ...)";
        return documentUrl.pathOrUrl();
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
        qWarning() << "document URL has to point to a file location or URL";
        return documentUrl.pathOrUrl();
    }
    if (documentUrl.isRelative() && (baseUrl.isEmpty() || baseUrl.isRelative())) {
        qWarning() << "base URL has to point to an absolute file location or URL if the document URL is relative";
        return documentUrl.pathOrUrl();
    }
    if (documentUrl.isRelative() && (documentUrl.scheme() != baseUrl.scheme() || (documentUrl.scheme() != QLatin1String("file") && documentUrl.host() != baseUrl.host()))) {
        qWarning() << "document URL and base URL do not match (protocol, host, ...), but necessary if the document URL is relative";
        return documentUrl.pathOrUrl();
    }

    /// Resolve the provided document URL to an absolute URL
    /// using the given base URL
    QUrl internalDocumentUrl = documentUrl;
    if (internalDocumentUrl.isRelative())
        internalDocumentUrl = baseUrl.resolved(internalDocumentUrl);

    return internalDocumentUrl.pathOrUrl();
}

QString AssociatedFiles::associateDocumentURL(const QUrl &document, QSharedPointer<Entry> &entry, const File *bibTeXFile, PathType pathType, const bool dryRun) {
    Q_ASSERT(bibTeXFile != NULL); // FIXME more graceful?

    const QUrl baseUrl = bibTeXFile->property(File::Url).toUrl();
    if (baseUrl.isEmpty() && pathType == ptRelative) {
        /// If no base URL was given but still a relative path was requested,
        /// revert choice and enforce the generation of an absolute one
        pathType = ptAbsolute;
    }

    const bool isLocal = urlIsLocal(document);
    const QString field = isLocal ? Entry::ftLocalFile : Entry::ftUrl;
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
    Q_ASSERT(bibTeXFile != NULL); // FIXME more graceful?

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
    Q_ASSERT(bibTeXFile != NULL); // FIXME more graceful?

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
        suffix = QLatin1String("html");
        filename.append(QLatin1Char('.')).append(suffix);
    }
    if (filename.isEmpty()) filename = internalSourceUrl.pathOrUrl().remove(QDir::separator()).remove(QLatin1Char('/')).remove(QLatin1Char(':')).remove(QLatin1Char('.')) + QLatin1String(".") + suffix;

    if (!bibTeXFile->hasProperty(File::Url)) return QUrl(); /// no valid URL set of BibTeX file object
    QUrl targetUrl = bibTeXFile->property(File::Url).toUrl();
    if (targetUrl.isEmpty()) return QUrl(); /// no valid URL set of BibTeX file object
    const QString targetPath = QFileInfo(targetUrl.path()).absolutePath();
    targetUrl.setPath(targetPath + QDir::separator() + (renameOperation == roEntryId ? entryId + QLatin1String(".") + suffix : (renameOperation == roUserDefined ? userDefinedFilename : filename)));

    if (!dryRun) { /// only if not pretending
        bool success = true;
        if (urlIsLocal(internalSourceUrl) && urlIsLocal(targetUrl)) {
            QFile(targetUrl.path()).remove();
            success &= QFile::copy(internalSourceUrl.path(), targetUrl.path()); // FIXME check if succeeded
            if (moveCopyOperation == mcoMove) {
                success &= QFile(internalSourceUrl.path()).remove();
            }
        } else {
            KIO::NetAccess::del(sourceUrl, widget); // FIXME non-blocking
            success &= KIO::NetAccess::file_copy(sourceUrl, targetUrl, widget); // FIXME non-blocking
            if (moveCopyOperation == mcoMove) {
                success &= KIO::NetAccess::del(sourceUrl, widget); // FIXME non-blocking
            }
        }

        if (!success) return QUrl(); ///< either copy/move or delete operation failed
    }

    return targetUrl;
}

