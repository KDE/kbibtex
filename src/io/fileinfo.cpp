/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileinfo.h"

#include <poppler-qt4.h>

#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QtConcurrentRun>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KStandardDirs>

#include "kbibtexnamespace.h"
#include "entry.h"

FileInfo::FileInfo()
{
    // TODO
}

const QString FileInfo::mimetypeOctetStream = QLatin1String("application/octet-stream");
const QString FileInfo::mimetypeHTML = QLatin1String("text/html");
const QString FileInfo::mimetypeBibTeX = QLatin1String("text/x-bibtex");
const QString FileInfo::mimetypeRIS = QLatin1String("application/x-research-info-systems");
const QString FileInfo::mimetypePDF = QLatin1String("application/pdf");

KMimeType::Ptr FileInfo::mimeTypeForUrl(const KUrl &url)
{
    static const QString invalidExtension = QLatin1String("XXXXXXXXXXXXXXXXXXXXXXX");
    static const KMimeType::Ptr mimetypeHTMLPtr(KMimeType::mimeType(mimetypeHTML));
    static const KMimeType::Ptr mimetypeOctetStreamPtr(KMimeType::mimeType(mimetypeOctetStream));
    static const KMimeType::Ptr mimetypeBibTeXPtr(KMimeType::mimeType(mimetypeBibTeX));
    static const KMimeType::Ptr mimetypePDFPtr(KMimeType::mimeType(mimetypePDF));
    /// Test if mime type for BibTeX is registered before determining file extension
    static const QString mimetypeBibTeXExt = mimetypeBibTeXPtr.isNull() ? invalidExtension : mimetypeBibTeXPtr->mainExtension().mid(1);
    static const KMimeType::Ptr mimetypeRISPtr(KMimeType::mimeType(mimetypeRIS));
    /// Test if mime type for RIS is registered before determining file extension
    static const QString mimetypeRISExt = mimetypeRISPtr.isNull() ? invalidExtension : mimetypeRISPtr->mainExtension().mid(1);
    /// Test if mime type for PDF is registered before determining file extension
    static const QString mimetypePDFExt = mimetypePDFPtr.isNull() ? invalidExtension : mimetypePDFPtr->mainExtension().mid(1);

    const QString extension = KMimeType::extractKnownExtension(url.fileName()).toLower();
    if (extension == mimetypeBibTeXExt)
        return mimetypeBibTeXPtr;
    else if (extension == mimetypeRISExt)
        return mimetypeRISPtr;
    else if (extension == mimetypePDFExt)
        return mimetypePDFPtr;
    // TODO other extensions

    /// Let the KDE subsystem guess the mime type
    KMimeType::Ptr result = KMimeType::findByUrl(url);
    /// Fall back to application/octet-stream if something goes wrong
    if (result.isNull())
        result = mimetypeOctetStreamPtr;

    /// In case that KDE could not determine mime type,
    /// do some educated guesses on our own
    if (result.isNull() || result->name() == mimetypeOctetStream) {
        if (url.protocol().startsWith(QLatin1String("http")))
            result = mimetypeHTMLPtr;
        // TODO more tests?
    }

    return result;
}

void FileInfo::urlsInText(const QString &text, TestExistence testExistence, const QString &baseDirectory, QList<KUrl> &result)
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
        KUrl url(doiUrlPrefix() + match.remove("\\"));
        if (url.isValid() && !result.contains(url))
            result << url;
        /// remove match from internal text to avoid duplicates
        internalText = internalText.left(pos) + internalText.mid(pos + match.length());
    }

    const QStringList fileList = internalText.split(KBibTeX::fileListSeparatorRegExp, QString::SkipEmptyParts);
    for (QStringList::ConstIterator filesIt = fileList.constBegin(); filesIt != fileList.constEnd(); ++filesIt) {
        internalText = *filesIt;

        /// If testing for the actual existence of a filename found in the text ...
        if (testExistence == TestExistenceYes) {
            /// If a base directory (e.g. the location of the parent .bib file) is given
            /// and the potential filename fragment is NOT an absolute path, ...
            if (!baseDirectory.isEmpty() &&
                    // TODO the following test assumes that absolute paths start
                    // with a dir separator, which may only be true on Unix/Linux,
                    // but not Windows. May be a test for 'first character is a letter,
                    // second is ":", third is "\"' may be necessary.
                    !internalText.startsWith(QDir::separator())) {
                /// To get the absolute path, prepend filename fragment with base directory
                const QString fullFilename = baseDirectory + QDir::separator() + internalText;
                const QFileInfo fileInfo(fullFilename);
                const KUrl url = KUrl::fromPath(fileInfo.canonicalFilePath());
                if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                    result << url;
                    /// Stop searching for URLs or filenames in current internal text
                    continue;
                }
            } else {
                /// Either the filename fragment is an absolute path OR no base directory
                /// was given (current working directory is assumed), ...
                const QFileInfo fileInfo(internalText);
                const KUrl url = KUrl::fromPath(fileInfo.canonicalFilePath());
                if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                    result << url;
                    /// stop searching for URLs or filenames in current internal text
                    continue;
                }
            }
        }

        /// extract URL from current field
        pos = 0;
        while ((pos = KBibTeX::urlRegExp.indexIn(internalText, pos)) != -1) {
            const QString match = KBibTeX::urlRegExp.cap(0);
            KUrl url(match);
            if (url.isValid() && (testExistence == TestExistenceNo || !url.isLocalFile() || QFileInfo(url.toLocalFile()).exists()) && !result.contains(url))
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
            if (url.isValid() && (testExistence == TestExistenceNo || !url.isLocalFile() || QFileInfo(url.toLocalFile()).exists()) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }
    }
}

QList<KUrl> FileInfo::entryUrls(const Entry *entry, const KUrl &bibTeXUrl, TestExistence testExistence)
{
    QList<KUrl> result;
    if (entry == NULL || entry->isEmpty())
        return result;

    if (entry->contains(Entry::ftDOI)) {
        const QString doi = PlainTextValue::text(entry->value(Entry::ftDOI));
        if (!doi.isEmpty() && KBibTeX::doiRegExp.indexIn(doi) == 0) {
            QString match = KBibTeX::doiRegExp.cap(0);
            KUrl url(doiUrlPrefix() + match.remove("\\"));
            result.append(url);
        }
    }
    static const QString etPMID = QLatin1String("pmid");
    if (entry->contains(etPMID)) {
        const QString pmid = PlainTextValue::text(entry->value(etPMID));
        bool ok = false;
        ok &= pmid.toInt(&ok) > 0;
        if (ok) {
            KUrl url(QLatin1String("https://www.ncbi.nlm.nih.gov/pubmed/") + pmid);
            result.append(url);
        }
    }
    static const QString etEPrint = QLatin1String("eprint");
    if (entry->contains(etEPrint)) {
        const QString eprint = PlainTextValue::text(entry->value(etEPrint));
        if (!eprint.isEmpty()) {
            KUrl url(QLatin1String("http://arxiv.org/search?query=") + eprint);
            result.append(url);
        }
    }

    const QString baseDirectory = bibTeXUrl.isValid() ? bibTeXUrl.directory() : QString();

    static const QRegExp regExpEscapedChars = QRegExp("\\\\+([&_~])");
    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        /// skip abstracts, they contain sometimes strange text fragments
        /// that are mistaken for URLs
        if (it.key().toLower() == Entry::ftAbstract) continue;

        Value v = it.value();

        for (Value::ConstIterator vit = v.constBegin(); vit != v.constEnd(); ++vit) {
            QString plainText = PlainTextValue::text(*(*vit));

            int pos = -1;
            while ((pos = regExpEscapedChars.indexIn(plainText, pos + 1)) != -1)
                plainText = plainText.replace(regExpEscapedChars.cap(0), regExpEscapedChars.cap(1));

            urlsInText(plainText, testExistence, baseDirectory, result);
        }
    }

    if (!baseDirectory.isEmpty()) {
        /// File types supported by "document preview"
        static const QStringList documentFileExtensions = QStringList() << ".pdf" << ".pdf.gz" << ".pdf.bz2" << ".ps" << ".ps.gz" << ".ps.bz2" << ".eps" << ".eps.gz" << ".eps.bz2" << ".html" << ".xhtml" << ".htm" << ".dvi" << ".djvu" << ".wwf" << ".jpeg" << ".jpg" << ".png" << ".gif" << ".tif" << ".tiff";

        /// check if in the same directory as the BibTeX file
        /// a PDF file exists which filename is based on the entry's id
        for (QStringList::ConstIterator extensionIt = documentFileExtensions.constBegin(); extensionIt != documentFileExtensions.constEnd(); ++extensionIt) {
            const QFileInfo fi(baseDirectory + QDir::separator() + entry->id() + *extensionIt);
            if (fi.exists()) {
                const KUrl url = KUrl::fromPath(fi.canonicalFilePath());
                if (!result.contains(url))
                    result << url;
            }
        }

        /// check if in the same directory as the BibTeX file there is a subdirectory
        /// similar to the BibTeX file's name and which contains a PDF file exists
        /// which filename is based on the entry's id
        QString basename = bibTeXUrl.fileName().remove(QRegExp("\\.[^.]{2,5}$"));
        QString directory = baseDirectory + QDir::separator() + basename;
        for (QStringList::ConstIterator extensionIt = documentFileExtensions.constBegin(); extensionIt != documentFileExtensions.constEnd(); ++extensionIt) {
            const QFileInfo fi(directory + QDir::separator() + entry->id() + *extensionIt);
            if (fi.exists()) {
                const KUrl url = KUrl::fromPath(fi.canonicalFilePath());
                if (!result.contains(url))
                    result << url;
            }
        }
    }

    return result;
}

QString FileInfo::pdfToText(const QString &pdfFilename)
{
    /// Build filename for text file where PDF file's plain text is cached
    static const QRegExp invalidChars("[^-a-z0-9_]", Qt::CaseInsensitive);
    QString textFilename = QString(pdfFilename).remove(invalidChars).append(QLatin1String(".txt")).prepend(KStandardDirs::locateLocal("cache", "pdftotext/"));

    /// First, check if there is a cache text file
    if (QFileInfo(textFilename).exists()) {
        /// Load text from cache file
        QFile f(textFilename);
        if (f.open(QFile::ReadOnly)) {
            QTextStream ts(&f);
            const QString text = ts.readAll();
            f.close();
            return text;
        }
    } else
        /// No cache text exists, so run text extraction in another thread
        QtConcurrent::run(extractPDFTextToCache, pdfFilename, textFilename);

    return QString();
}

void FileInfo::extractPDFTextToCache(const QString &pdfFilename, const QString &cacheFilename) {
    /// In case of multiple calls, skip text extraction if cache file already exists
    if (QFile(cacheFilename).exists()) return;

    QString text;
    QStringList msgList;

    /// Load PDF file through Poppler
    Poppler::Document *doc = Poppler::Document::load(pdfFilename);
    if (doc != NULL) {
        static const int maxPages = 64;
        /// Build text by appending each page's text
        for (int i = 0; i < qMin(maxPages, doc->numPages()); ++i)
            text.append(doc->page(i)->text(QRect())).append(QLatin1String("\n\n"));
        if (doc->numPages() > maxPages)
            msgList << QString(QLatin1String("### Skipped %1 pages as PDF file contained too many pages (limit is %2 pages) ###")).arg(doc->numPages() - maxPages).arg(maxPages);
        delete doc;
    } else
        msgList << QLatin1String("### Skipped as file could not be opened as PDF file ###");

    /// Save text in cache file
    QFile f(cacheFilename);
    if (f.open(QFile::WriteOnly)) {
        static const int maxCharacters = 1 << 18;
        QTextStream ts(&f);
        ts << text.left(maxCharacters); ///< keep only the first 2^18 many characters

        if (text.length() > maxCharacters)
            msgList << QString(QLatin1String("### Text too long, skipping %1 characters ###")).arg(text.length() - maxCharacters);
        /// Write all messages (warnings) to end of text file
        foreach(const QString &msg, msgList)
           ts << endl << msg;

        f.close();
    }
}

QString FileInfo::doiUrlPrefix()
{
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    static const QString configGroupNameNetworking(QLatin1String("Networking"));
    static const QString keyDOIUrlPrefix(QLatin1String("DOIUrlPrefix"));
    KConfigGroup configGroup(config, configGroupNameNetworking);
    return configGroup.readEntry(keyDOIUrlPrefix, KBibTeX::doiUrlPrefix);
}
