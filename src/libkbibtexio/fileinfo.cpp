/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <poppler/qt4/poppler-qt4.h>

#include <QFileInfo>
#include <QDir>
#include <QTextStream>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include <kbibtexnamespace.h>
#include <entry.h>
#include "fileinfo.h"

static const QRegExp regExpEscapedChars = QRegExp("\\\\+([&_~])");

/// File types supported by "document preview"
static const QStringList documentFileExtensions = QStringList() << ".pdf" << ".pdf.gz" << ".pdf.bz2" << ".ps" << ".ps.gz" << ".ps.bz2" << ".eps" << ".eps.gz" << ".eps.bz2" << ".html" << ".xhtml" << ".htm" << ".dvi" << ".djvu" << ".wwf" << ".jpeg" << ".jpg" << ".png" << ".gif" << ".tif" << ".tiff";

FileInfo::FileInfo()
{
    // TODO
}

const QString FileInfo::mimetypeBibTeX = QLatin1String("text/x-bibtex");
const QString FileInfo::mimetypeRIS = QLatin1String("application/x-research-info-systems");

KMimeType::Ptr FileInfo::mimeTypeForUrl(const KUrl &url)
{
    static const KMimeType::Ptr mimetypeBibTeXPtr(KMimeType::mimeType(mimetypeBibTeX));
    static const QString mimetypeBibTeXExt = mimetypeBibTeXPtr->mainExtension().mid(1);
    static const KMimeType::Ptr mimetypeRISPtr(KMimeType::mimeType(mimetypeRIS));
    static const QString mimetypeRISExt = mimetypeRISPtr->mainExtension().mid(1);

    const QString extension = KMimeType::extractKnownExtension(url.fileName()).toLower();

    if (extension == mimetypeBibTeXExt)
        return mimetypeBibTeXPtr;
    else if (extension == mimetypeRISExt)
        return mimetypeRISPtr;
    // TODO other extensions
    else
        return KMimeType::findByUrl(url);
}

void FileInfo::urlsInText(const QString &text, TestExistance testExistance, const QString &baseDirectory, QList<KUrl> &result)
{
    if (text.isEmpty())
        return;

    /// DOI identifiers have to extracted first as KBibTeX::fileListSeparatorRegExp
    /// contains characters that can be part of a DOI (e.g. ';') and thus could split
    /// a DOI in between.
    QString internalText = text;
    int pos = 0;
    while ((pos = KBibTeX::doiRegExp.indexIn(internalText, pos)) != -1) {
        QString match = KBibTeX::doiRegExp.cap(0);
        KUrl url(doiUrlPrefix() + match.replace("\\", ""));
        if (url.isValid() && !result.contains(url))
            result << url;
        /// remove match from internal text to avoid duplicates
        internalText = internalText.left(pos) + internalText.mid(pos + match.length());
    }

    const QStringList fileList = internalText.split(KBibTeX::fileListSeparatorRegExp, QString::SkipEmptyParts);
    for (QStringList::ConstIterator filesIt = fileList.constBegin(); filesIt != fileList.constEnd(); ++filesIt) {
        internalText = *filesIt;

        if (testExistance == TestExistanceYes) {
            QFileInfo fileInfo(internalText);
            KUrl url = KUrl(fileInfo.filePath());
            if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                /// text points to existing file (most likely with absolute path)
                result << url;
                /// stop searching for urls or filenames in current internal text
                continue;
            } else if (!baseDirectory.isEmpty()) {
                const QString fullFilename = baseDirectory + QDir::separator() + internalText;
                fileInfo = QFileInfo(fullFilename);
                url = KUrl(fileInfo.filePath());
                if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                    /// text points to existing file in base directory
                    result << url;
                    /// stop searching for urls or filenames in current internal text
                    continue;
                }
            }
        }

        /// extract URL from current field
        pos = 0;
        while ((pos = KBibTeX::urlRegExp.indexIn(internalText, pos)) != -1) {
            const QString match = KBibTeX::urlRegExp.cap(0);
            KUrl url(match);
            if (url.isValid() && (testExistance == TestExistanceNo || !url.isLocalFile() || QFileInfo(url.toLocalFile()).exists()) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// explicitly check URL entry, may be an URL even if http:// or alike is missing
        pos = 0;
        while ((pos = KBibTeX::domainNameRegExp.indexIn(internalText, pos)) > -1) {
            int pos2 = internalText.indexOf(" ", pos + 1);
            if (pos2 < 0) pos2 = internalText.length();
            QString match = internalText.mid(pos, pos2 - pos);
            KUrl url("http://" + match);
            if (url.isValid() && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// extract general file-like patterns
        pos = 0;
        while ((pos = KBibTeX::fileRegExp.indexIn(internalText, pos)) != -1) {
            QString match = KBibTeX::fileRegExp.cap(0);
            KUrl url(match);
            if (url.isValid() && (testExistance == TestExistanceNo || !url.isLocalFile() || QFileInfo(url.toLocalFile()).exists()) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }
    }
}

QList<KUrl> FileInfo::entryUrls(const Entry *entry, const KUrl &bibTeXUrl, TestExistance testExistance)
{
    QList<KUrl> result;
    if (entry == NULL || entry->isEmpty())
        return result;

    const QString baseDirectory = bibTeXUrl.isValid() ? bibTeXUrl.directory() : QString::null;

    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        /// skip abstracts, they contain sometimes strange text fragments
        /// that are mistaken for URLs
        if (it.key().toLower() == Entry::ftAbstract) continue;

        Value v = it.value();

        for (Value::ConstIterator vit = v.constBegin(); vit != v.constEnd(); ++vit) {
            QString plainText = PlainTextValue::text(*(*vit), NULL);

            int pos = -1;
            while ((pos = regExpEscapedChars.indexIn(plainText, pos + 1)) != -1)
                plainText = plainText.replace(regExpEscapedChars.cap(0), regExpEscapedChars.cap(1));

            urlsInText(plainText, testExistance, baseDirectory, result);
        }
    }

    if (!baseDirectory.isEmpty()) {
        /// check if in the same directory as the BibTeX file
        /// a PDF file exists which filename is based on the entry's id
        for (QStringList::ConstIterator extensionIt = documentFileExtensions.constBegin(); extensionIt != documentFileExtensions.constEnd(); ++extensionIt) {
            KUrl url(baseDirectory + QDir::separator() + entry->id() + *extensionIt);
            url.setProtocol(QLatin1String("file"));
            if (QFileInfo(url.toLocalFile()).exists() && !result.contains(url))
                result << url;
        }

        /// check if in the same directory as the BibTeX file there is a subdirectory
        /// similar to the BibTeX file's name and which contains a PDF file exists
        /// which filename is based on the entry's id
        QString basename = bibTeXUrl.fileName().replace(QRegExp("\\.[^.]{2,5}$"), "");
        QString directory = baseDirectory + QDir::separator() + basename;
        for (QStringList::ConstIterator extensionIt = documentFileExtensions.constBegin(); extensionIt != documentFileExtensions.constEnd(); ++extensionIt) {
            KUrl url(directory + QDir::separator() + entry->id() + *extensionIt);
            url.setProtocol(QLatin1String("file"));
            if (QFileInfo(url.toLocalFile()).exists() && !result.contains(url))
                result << url;
        }
    }

    return result;
}

QString FileInfo::pdfToText(const QString &pdfFilename)
{
    /// Build filename for text file where PDF file's plain text is cached
    static const QRegExp invalidChars("[^-a-z0-9_]", Qt::CaseInsensitive);
    QString textFilename = QString(pdfFilename).replace(invalidChars, "").append(QLatin1String(".txt")).prepend(KStandardDirs::locateLocal("cache", "pdftotext/"));

    /// Initialize return value
    QString text = QString::null;

    /// First, check if there is a cache text file
    if (QFileInfo(textFilename).exists()) {
        /// Load text from cache file
        QFile f(textFilename);
        if (f.open(QFile::ReadOnly)) {
            QTextStream ts(&f);
            text = ts.readAll();
            f.close();
        }
    }

    /// Either no cache text file existed or could not load text from it
    if (text.isNull()) {
        /// Load PDF file through Poppler
        Poppler::Document *doc = Poppler::Document::load(pdfFilename);
        if (doc != NULL) {
            /// Build text by appending each page's text
            text = QLatin1String("");
            for (int i = 0; i < doc->numPages(); ++i)
                text.append(doc->page(i)->text(QRect())).append(QLatin1String("\n\n"));
            delete doc;

            /// Save text in cache file
            QFile f(textFilename);
            if (f.open(QFile::WriteOnly)) {
                QTextStream ts(&f);
                ts << text;
                f.close();
            }
        }
    }

    return text;
}

QString FileInfo::doiUrlPrefix()
{
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    static const QString configGroupNameNetworking(QLatin1String("Networking"));
    static const QString keyDOIUrlPrefix(QLatin1String("DOIUrlPrefix"));
    KConfigGroup configGroup(config, configGroupNameNetworking);
    return configGroup.readEntry(keyDOIUrlPrefix, KBibTeX::doiUrlPrefix);
}
