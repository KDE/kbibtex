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
#include <KDebug>

bool AssociatedFiles::urlIsLocal(const QUrl &url)
{
    const QString scheme = url.scheme();
    /// Test various schemes such as "http", "https", "ftp", ...
    return !scheme.startsWith(QLatin1String("http")) && !scheme.startsWith(QLatin1String("ftp")) && !scheme.startsWith(QLatin1String("sftp")) && !scheme.startsWith(QLatin1String("fish")) && !scheme.startsWith(QLatin1String("webdav")) && scheme != QLatin1String("smb");
}

QString AssociatedFiles::relativeFilename(const QUrl &file, const QUrl &baseUrl) {
    kDebug() << "file=" << file.toString();
    kDebug() << "baseUrl=" << baseUrl.toString();

    QUrl internalFile = file;
    if (internalFile.isRelative()) internalFile = baseUrl.resolved(internalFile);
    kDebug() << "internalFile=" << internalFile.toString();

    const QFileInfo baseUrlInfo = baseUrl.isEmpty() ? QFileInfo() : QFileInfo(baseUrl.path());
    const QString baseCanonicalPath = baseUrlInfo.absolutePath();
    kDebug() << "baseCanonicalPath=" << baseCanonicalPath;
    const QFileInfo fileInfo(internalFile.path());
    const QString fileCanonicalName = fileInfo.absoluteFilePath();
    kDebug() << "fileCanonicalName=" << fileCanonicalName;
    if (urlIsLocal(internalFile)) {
        /// Document URL points to a local file
        if (!baseUrl.isEmpty()) {
            if (baseCanonicalPath.endsWith(QDir::separator()) && fileCanonicalName.startsWith(baseCanonicalPath)) {
                const QString relativeFilename = fileCanonicalName.mid(baseCanonicalPath.length());
                kDebug() << "relativeFilename=" << relativeFilename;
                return relativeFilename;
            } else if (fileCanonicalName.startsWith(baseCanonicalPath + QDir::separator())) {
                const QString relativeFilename = fileCanonicalName.mid(baseCanonicalPath.length() + 1);
                kDebug() << "relativeFilename=" << relativeFilename;
                return relativeFilename;
            }
        }
        /// Fallback if relative filename cannot be determined
        kDebug() << "fileCanonicalName=" << fileCanonicalName;
        return fileCanonicalName;
    } else {
        /// Document URL points to a remote location
        internalFile.setPath(fileCanonicalName);
        const QString internalFilePath = internalFile.toString();
        kDebug() << "internalFilePath=" << internalFilePath;
        QUrl internalBaseUrl = baseUrl;
        internalBaseUrl.setPath(baseCanonicalPath);
        const QString internalBasePath = internalBaseUrl.toString();
        kDebug() << "internalBasePath=" << internalBasePath;

        if (!internalBaseUrl.isEmpty()) {
            if (internalBasePath.endsWith(QDir::separator()) && internalFilePath.startsWith(internalBasePath)) {
                const QString relativeFilename = internalFilePath.mid(internalBasePath.length());
                kDebug() << "relativeFilename=" << relativeFilename;
                return relativeFilename;
            } else if (internalFilePath.startsWith(internalBasePath + QDir::separator())) {
                const QString relativeFilename = internalFilePath.mid(internalBasePath.length() + 1);
                kDebug() << "relativeFilename=" << relativeFilename;
                return relativeFilename;
            }
        }

        /// Fallback if relative filename cannot be determined
        kDebug() << "internalFilePath=" << internalFilePath;
        return internalFilePath;
    }
}

QString AssociatedFiles::absoluteFilename(const QUrl &file, const QUrl &baseUrl) {
    kDebug() << "file=" << file.toString();
    QUrl internalFile = file;
    if (internalFile.isRelative()) internalFile = baseUrl.resolved(internalFile);
    kDebug() << "internalFile=" << internalFile.toString();

    const QFileInfo fileInfo(internalFile.path());
    if (urlIsLocal(internalFile)) {
        /// Document URL points to a local file
        kDebug() << "fileInfo.absoluteFilePath()=" << fileInfo.absoluteFilePath();
        return fileInfo.absoluteFilePath();
    } else {
        /// Document URL points to a remote location
        internalFile.setPath(fileInfo.absoluteFilePath());
        kDebug() << "internalFile=" << internalFile.toString();
        return internalFile.toString();
    }
}

QString AssociatedFiles::associateDocumentURL(const QUrl &document, QSharedPointer<Entry> &entry, const File *bibTeXFile, PathType pathType, const bool dryRun) {
    Q_ASSERT(bibTeXFile != NULL); // FIXME more graceful?

    kDebug() << "document=" << document.toString();
    kDebug() << "urlIsLocal(document)=" << urlIsLocal(document);
    kDebug() << "entry.id()=" << entry->id();
    kDebug() << "bibTeXFile.count()=" << bibTeXFile->count();
    kDebug() << "pathType=" << pathType;

    const QUrl baseUrl = bibTeXFile->property(File::Url).toUrl();
    kDebug() << "bibTeXFile->property(File::Url).toUrl()=" << baseUrl.toString();
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
        kDebug() << "Adding url" << finalUrl;
        for (QMap<QString, Value>::ConstIterator it = entry->constBegin(); !alreadyContained && it != entry->constEnd(); ++it) {
            const Value v = it.value();
            for (Value::ConstIterator vit = v.constBegin(); !alreadyContained && vit != v.constEnd(); ++vit) {
                if (PlainTextValue::text(*vit, bibTeXFile) == finalUrl)
                    alreadyContained = true;
            }
        }
        kDebug() << "alreadyContained=" << alreadyContained;
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

    kDebug() << "document=" << document.toString();
    kDebug() << "urlIsLocal(document)=" << urlIsLocal(document);
    kDebug() << "bibTeXFile.count()=" << bibTeXFile->count();
    kDebug() << "pathType=" << pathType;

    const QUrl baseUrl = bibTeXFile->property(File::Url).toUrl();
    kDebug() << "bibTeXFile->property(File::Url).toUrl()=" << baseUrl.toString();
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
    kDebug() << "internalSourceUrl=" << internalSourceUrl.toString();
    kDebug() << "entry.id()=" << entryId;
    kDebug() << "bibTeXFile.count()=" << bibTeXFile->count();
    kDebug() << "bibTeXFile->property(File::Url).toUrl()=" << baseUrl.toString();
    kDebug() << "renameOperation=" << renameOperation;
    kDebug() << "moveCopyOperation=" << moveCopyOperation;
    kDebug() << "userDefinedFilename=" << userDefinedFilename;

    const QFileInfo internalSourceInfo(internalSourceUrl.path());
    QString filename = internalSourceInfo.fileName();
    QString suffix = internalSourceInfo.suffix();
    if (suffix.isEmpty()) {
        suffix = QLatin1String("html");
        filename.append(QLatin1Char('.')).append(suffix);
    }
    kDebug() << "suffix=" << suffix;
    kDebug() << "filename=" << filename;
    if (filename.isEmpty()) filename = internalSourceUrl.toString().remove(QDir::separator()).remove(QLatin1Char('/')).remove(QLatin1Char(':')).remove(QLatin1Char('.')) + QLatin1String(".") + suffix;
    kDebug() << "filename=" << filename;

    if (!bibTeXFile->hasProperty(File::Url)) return QUrl(); /// no valid URL set of BibTeX file object
    QUrl targetUrl = bibTeXFile->property(File::Url).toUrl();
    if (targetUrl.isEmpty()) return QUrl(); /// no valid URL set of BibTeX file object
    const QString targetPath = QFileInfo(targetUrl.path()).absolutePath();
    targetUrl.setPath(targetPath + QDir::separator() + (renameOperation == roEntryId ? entryId + QLatin1String(".") + suffix : (renameOperation == roUserDefined ? userDefinedFilename : filename)));
    kDebug() << "targetUrl=" << targetUrl.toString();

    if (!dryRun) { /// only if not pretending
        bool success = true;
        if (urlIsLocal(internalSourceUrl) && urlIsLocal(targetUrl)) {
            QFile(targetUrl.path()).remove();
            success &= QFile::copy(internalSourceUrl.path(), targetUrl.path()); // FIXME check if succeeded
            kDebug() << "copy=" << success << sourceUrl.path() << targetUrl.path();
            if (moveCopyOperation == mcoMove) {
                success &= QFile(internalSourceUrl.path()).remove();
                kDebug() << "remove=" << success;
            }
        } else {
            KIO::NetAccess::del(sourceUrl, widget); // FIXME non-blocking
            success &= KIO::NetAccess::file_copy(sourceUrl, targetUrl, widget); // FIXME non-blocking
            kDebug() << "KIO copy=" << success;
            if (moveCopyOperation == mcoMove) {
                success &= KIO::NetAccess::del(sourceUrl, widget); // FIXME non-blocking
                kDebug() << "KIO del=" << success;
            }
        }

        if (!success) return QUrl(); ///< either copy/move or delete operation failed
    }

    return targetUrl;
}
