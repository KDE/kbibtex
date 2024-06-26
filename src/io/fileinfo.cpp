/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "fileinfo.h"

#ifdef HAVE_POPPLERQT5
#include <poppler-qt5.h>
#endif // HAVE_POPPLERQT5

#include <QFileInfo>
#include <QMimeDatabase>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#ifdef HAVE_QTCONCURRENT
#include <QtConcurrentRun>
#endif // HAVE_QTCONCURRENT

#include <KBibTeX>
#include <Entry>
#include "logging_io.h"

FileInfo::FileInfo()
{
    /// nothing
}

const QString FileInfo::mimetypeOctetStream = QStringLiteral("application/octet-stream");
const QString FileInfo::mimetypeHTML = QStringLiteral("text/html");
const QString FileInfo::mimetypeBibTeX = QStringLiteral("text/x-bibtex");
const QString FileInfo::mimetypeRIS = QStringLiteral("application/x-research-info-systems");
const QString FileInfo::mimetypePDF = QStringLiteral("application/pdf");

QMimeType FileInfo::mimeTypeForUrl(const QUrl &url)
{
    if (!url.isValid()) {
        qCWarning(LOG_KBIBTEX_IO) << "Cannot determine mime type for empty or invalid QUrl";
        return QMimeType(); ///< invalid input gives invalid mime type
    }

    static const QMimeDatabase db;
    static const QMimeType mtHTML(db.mimeTypeForName(mimetypeHTML));
    static const QMimeType mtOctetStream(db.mimeTypeForName(mimetypeOctetStream));
    static const QMimeType mtBibTeX(db.mimeTypeForName(mimetypeBibTeX));
    static const QMimeType mtPDF(db.mimeTypeForName(mimetypePDF));
    static const QMimeType mtRIS(db.mimeTypeForName(mimetypeRIS));

    /// Test if mime type for BibTeX is registered before determining file extension
    static const QString mimetypeBibTeXExt = mtBibTeX.preferredSuffix();
    /// Test if mime type for RIS is registered before determining file extension
    static const QString mimetypeRISExt = mtRIS.preferredSuffix();
    /// Test if mime type for PDF is registered before determining file extension
    static const QString mimetypePDFExt = mtPDF.preferredSuffix();

    const QString extension = db.suffixForFileName(url.fileName()).toLower();
    /// First, check preferred suffixes
    if (mtBibTeX.isValid() && extension == mimetypeBibTeXExt)
        return mtBibTeX;
    else if (mtRIS.isValid() && extension == mimetypeRISExt)
        return mtRIS;
    else if (extension == mimetypePDFExt)
        return mtPDF;
    /// Second, check any other suffixes
    else if (mtBibTeX.isValid() && mtBibTeX.suffixes().contains(extension))
        return mtBibTeX;
    else if (mtRIS.isValid() && mtRIS.suffixes().contains(extension))
        return mtRIS;
    else if (mtPDF.suffixes().contains(extension))
        return mtPDF;

    /// Let the KDE subsystem guess the mime type
    QMimeType result = db.mimeTypeForUrl(url);
    /// Fall back to application/octet-stream if something goes wrong
    if (!result.isValid())
        result = mtOctetStream;

    /// In case that KDE could not determine mime type,
    /// do some educated guesses on our own
    if (result.name() == mimetypeOctetStream) {
        if (url.scheme().startsWith(QStringLiteral("http")))
            result = mtHTML;
        // TODO more tests?
    }

    return result;
}

void FileInfo::urlsInText(const QString &text, const TestExistence testExistence, const QString &baseDirectory, QSet<QUrl> &result)
{
    if (text.isEmpty())
        return;

    /// DOI identifiers have to extracted first as KBibTeX::fileListSeparatorRegExp
    /// contains characters that can be part of a DOI (e.g. ';') and thus could split
    /// a DOI in between.
    QString internalText = text;
    int pos = 0;
    QRegularExpressionMatch doiRegExpMatch;
    while ((doiRegExpMatch = KBibTeX::doiRegExp.match(internalText, pos)).hasMatch()) {
        pos = doiRegExpMatch.capturedStart(QStringLiteral("doi"));
        QString doiMatch = doiRegExpMatch.captured(QStringLiteral("doi"));
        const int semicolonHttpPos = doiMatch.indexOf(QStringLiteral(";http"));
        if (semicolonHttpPos > 0) doiMatch = doiMatch.left(semicolonHttpPos);
        const QUrl url(KBibTeX::doiUrlPrefix + QString(doiMatch).remove(QStringLiteral("\\")));
        if (url.isValid() && !result.contains(url))
            result << url;
        /// remove match from internal text to avoid duplicates

        /// Cut away any URL that may be right before found DOI number:
        /// For example, if DOI '10.1000/38-abc' was found in
        ///   'Lore ipsum http://doi.example.org/10.1000/38-abc Lore ipsum'
        /// also remove 'http://doi.example.org/' from the text, keeping only
        ///   'Lore ipsum  Lore ipsum'
        static const QRegularExpression genericDoiUrlPrefix(QStringLiteral("http[s]?://[a-z0-9./-]+/$")); ///< looks like an URL
        const QRegularExpressionMatch genericDoiUrlPrefixMatch = genericDoiUrlPrefix.match(internalText.left(pos));
        if (genericDoiUrlPrefixMatch.hasMatch())
            /// genericDoiUrlPrefixMatch.captured(0) may contain (parts of) DOI
            internalText = internalText.left(genericDoiUrlPrefixMatch.capturedStart(0)) + internalText.mid(pos + doiMatch.length());
        else
            internalText = internalText.left(pos) + internalText.mid(pos + doiMatch.length());
    }

#if QT_VERSION >= 0x050e00
    const QStringList fileList = internalText.split(KBibTeX::fileListSeparatorRegExp, Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
    const QStringList fileList = internalText.split(KBibTeX::fileListSeparatorRegExp, QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
    for (const QString &text : fileList) {
        internalText = text;

        /// If testing for the actual existence of a filename found in the text ...
        if (testExistence == TestExistence::Yes) {
            /// If a base directory (e.g. the location of the parent .bib file) is given
            /// and the potential filename fragment is NOT an absolute path, ...
            if (internalText.startsWith(QStringLiteral("~") + QDir::separator())) {
                const QString fullFilename = QDir::homePath() + internalText.mid(1);
                const QFileInfo fileInfo(fullFilename);
                const QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
                if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                    result << url;
                    /// Stop searching for URLs or filenames in current internal text
                    continue;
                }
            } else if (!baseDirectory.isEmpty() &&
                       // TODO the following test assumes that absolute paths start
                       // with a dir separator, which may only be true on Unix/Linux,
                       // but not Windows. May be a test for 'first character is a letter,
                       // second is ":", third is "\"' may be necessary.
                       !internalText.startsWith(QDir::separator())) {
                /// To get the absolute path, prepend filename fragment with base directory
                const QString fullFilename = baseDirectory + QDir::separator() + internalText;
                const QFileInfo fileInfo(fullFilename);
                const QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
                if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                    result << url;
                    /// Stop searching for URLs or filenames in current internal text
                    continue;
                }
            } else {
                /// Either the filename fragment is an absolute path OR no base directory
                /// was given (current working directory is assumed), ...
                const QFileInfo fileInfo(internalText);
                const QUrl url = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
                if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                    result << url;
                    /// stop searching for URLs or filenames in current internal text
                    continue;
                }
            }
        }

        /// extract URL from current field
        pos = 0;
        QRegularExpressionMatch urlRegExpMatch;
        while ((urlRegExpMatch = KBibTeX::urlRegExp.match(internalText, pos)).hasMatch()) {
            pos = urlRegExpMatch.capturedStart(0);
            const QString match = urlRegExpMatch.captured(0);
            QUrl url(match);
            if (url.isValid() && (testExistence == TestExistence::No || !url.isLocalFile() || QFileInfo::exists(url.toLocalFile())) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// explicitly check URL entry, may be an URL even if http:// or alike is missing
        pos = 0;
        QRegularExpressionMatch domainNameRegExpMatch;
        while ((domainNameRegExpMatch = KBibTeX::domainNameRegExp.match(internalText, pos)).hasMatch()) {
            pos = domainNameRegExpMatch.capturedStart(0);
            /// URL ends either at space or at string's end
            int pos2 = internalText.indexOf(QStringLiteral(" "), pos + 1);
            if (pos2 < 0) pos2 = internalText.length();
            QString match = internalText.mid(pos, pos2 - pos);
            const QUrl url(QStringLiteral("https://") + match);
            if (url.isValid() && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// extract general file-like patterns
        pos = 0;
        QRegularExpressionMatch fileRegExpMatch;
        while ((fileRegExpMatch = KBibTeX::fileRegExp.match(internalText, pos)).hasMatch()) {
            pos = fileRegExpMatch.capturedStart(0);
            const QString match = fileRegExpMatch.captured(0);
            const QFileInfo fi(match);
            const QUrl url = QUrl::fromLocalFile(!match.startsWith(QStringLiteral("/")) && !match.startsWith(QStringLiteral("http")) && fi.isRelative() && !baseDirectory.isEmpty() ? baseDirectory + QStringLiteral("/") + match : match);
            if (url.isValid() && (testExistence == TestExistence::No || QFileInfo::exists(url.toLocalFile())) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }
    }
}

QSet<QUrl> FileInfo::entryUrls(const QSharedPointer<const Entry> &entry, const QUrl &bibTeXUrl, TestExistence testExistence)
{
    QSet<QUrl> result;
    if (entry.isNull() || entry->isEmpty())
        return result;

    const QString id = entry->id();
    if (id.length() > 4) {
        /// Sometimes the entry id contains or is actually a DOI number
        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(id);
        if (doiRegExpMatch.hasMatch()) {
            const QString match = doiRegExpMatch.captured(QStringLiteral("doi")).remove(QStringLiteral("\\"));
            QUrl url(KBibTeX::doiUrlPrefix + match);
            result.insert(url);
        }
    }
    if (entry->contains(Entry::ftDOI)) {
        const QString doi = PlainTextValue::text(entry->value(Entry::ftDOI));
        QRegularExpressionMatch doiRegExpMatch;
        if (!doi.isEmpty() && (doiRegExpMatch = KBibTeX::doiRegExp.match(doi)).hasMatch()) {
            const QString match = doiRegExpMatch.captured(QStringLiteral("doi")).remove(QStringLiteral("\\"));
            QUrl url(KBibTeX::doiUrlPrefix + match);
            result.insert(url);
        }
    }
    static const QString etPMID = QStringLiteral("pmid");
    if (entry->contains(etPMID)) {
        const QString pmid = PlainTextValue::text(entry->value(etPMID));
        bool ok = false;
        ok &= pmid.toInt(&ok) > 0;
        if (ok) {
            QUrl url(QStringLiteral("https://www.ncbi.nlm.nih.gov/pubmed/") + pmid);
            result.insert(url);
        }
    }
    static const QString etEPrint = QStringLiteral("eprint");
    if (entry->contains(etEPrint)) {
        const QString eprint = PlainTextValue::text(entry->value(etEPrint));
        if (!eprint.isEmpty()) {
            QUrl url(QStringLiteral("https://arxiv.org/search?query=") + eprint);
            result.insert(url);
        }
    }

    const QString baseDirectory = bibTeXUrl.isValid() ? bibTeXUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path() : QString();

    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        /// skip abstracts, they contain sometimes strange text fragments
        /// that are mistaken for URLs
        if (it.key().toLower() == Entry::ftAbstract) continue;

        const Value v = it.value();
        for (const auto &valueItem : v) {
            QString plainText = PlainTextValue::text(*valueItem);

            static const QRegularExpression regExpEscapedChars = QRegularExpression(QStringLiteral("\\\\+([&_~])"));
            plainText.replace(regExpEscapedChars, QStringLiteral("\\1"));

            urlsInText(plainText, testExistence, baseDirectory, result);
        }
    }

    if (!baseDirectory.isEmpty()) {
        /// File types supported by "document preview"
        static const QStringList documentFileExtensions {QStringLiteral(".pdf"), QStringLiteral(".pdf.gz"), QStringLiteral(".pdf.bz2"), QStringLiteral(".ps"), QStringLiteral(".ps.gz"), QStringLiteral(".ps.bz2"), QStringLiteral(".eps"), QStringLiteral(".eps.gz"), QStringLiteral(".eps.bz2"), QStringLiteral(".html"), QStringLiteral(".xhtml"), QStringLiteral(".htm"), QStringLiteral(".dvi"), QStringLiteral(".djvu"), QStringLiteral(".wwf"), QStringLiteral(".jpeg"), QStringLiteral(".jpg"), QStringLiteral(".png"), QStringLiteral(".gif"), QStringLiteral(".tif"), QStringLiteral(".tiff")};
        result.reserve(result.size() + documentFileExtensions.size() * 2);

        /// check if in the same directory as the BibTeX file
        /// a PDF file exists which filename is based on the entry's id
        for (const QString &extension : documentFileExtensions) {
            const QFileInfo fi(baseDirectory + QDir::separator() + entry->id() + extension);
            if (fi.exists()) {
                const QUrl url = QUrl::fromLocalFile(fi.absoluteFilePath());
                if (!result.contains(url))
                    result << url;
            }
        }

        /// Check if in the same directory as the BibTeX file there is a subdirectory
        /// similar to the BibTeX file's name and which contains a PDF file exists
        /// which filename is based on the entry's id
        const QFileInfo filenameInfo(bibTeXUrl.fileName());
        const QString ending = filenameInfo.completeSuffix();
        QString directory = baseDirectory + QDir::separator() + bibTeXUrl.fileName();
        directory.chop(ending.length() + 1);
        const QFileInfo fi(directory);
        if (fi.isDir())
            for (const QString &extension : documentFileExtensions) {
                const QFileInfo fi(directory + QDir::separator() + entry->id() + extension);
                if (fi.exists()) {
                    const QUrl url = QUrl::fromLocalFile(fi.absoluteFilePath());
                    if (!result.contains(url))
                        result << url;
                }
            }
    }

    return result;
}

#ifdef HAVE_POPPLERQT5
QString FileInfo::pdfToText(const QString &pdfFilename)
{
    /// Build filename for text file where PDF file's plain text is cached
    const QString cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/pdftotext");
    if (!QDir(cacheDirectory).exists() && !QDir::home().mkdir(cacheDirectory))
        /// Could not create cache directory
        return QString();
    static const QRegularExpression invalidChars(QStringLiteral("[^-a-z0-9_]"), QRegularExpression::CaseInsensitiveOption);
    const QString textFilename = QString(pdfFilename).remove(invalidChars).append(QStringLiteral(".txt")).prepend(QStringLiteral("/")).prepend(cacheDirectory);

    /// First, check if there is a cache text file
    if (QFileInfo::exists(textFilename)) {
        /// Load text from cache file
        QFile f(textFilename);
        if (f.open(QFile::ReadOnly)) {
            const QString text = QString::fromUtf8(f.readAll());
            f.close();
            return text;
        }
    } else {
#ifdef HAVE_QTCONCURRENT
        /// No cache file exists, so run text extraction in another thread
        QtConcurrent::run(extractPDFTextToCache, pdfFilename, textFilename);
#else // HAVE_QTCONCURRENT
        extractPDFTextToCache(pdfFilename, textFilename);
#endif // HAVE_QTCONCURRENT
    }

    return QString();
}

void FileInfo::extractPDFTextToCache(const QString &pdfFilename, const QString &cacheFilename) {
    /// In case of multiple calls, skip text extraction if cache file already exists
    if (QFile(cacheFilename).exists()) return;

    QString text;
    QStringList msgList;

    /// Load PDF file through Poppler
    Poppler::Document *doc = Poppler::Document::load(pdfFilename);
    if (doc != nullptr) {
        static const int maxPages = 64;
        /// Build text by appending each page's text
        for (int i = 0; i < qMin(maxPages, doc->numPages()); ++i)
            text.append(doc->page(i)->text(QRect())).append(QStringLiteral("\n\n"));
        if (doc->numPages() > maxPages)
            msgList << QString(QStringLiteral("### Skipped %1 pages as PDF file contained too many pages (limit is %2 pages) ###")).arg(doc->numPages() - maxPages).arg(maxPages);
        delete doc;
    } else
        msgList << QStringLiteral("### Skipped as file could not be opened as PDF file ###");

    /// Save text in cache file
    QFile f(cacheFilename);
    if (f.open(QFile::WriteOnly)) {
        static const int maxCharacters = 1 << 18;
        f.write(text.left(maxCharacters).toUtf8()); ///< keep only the first 2^18 many characters

        if (text.length() > maxCharacters)
            msgList << QString(QStringLiteral("### Text too long, skipping %1 characters ###")).arg(text.length() - maxCharacters);
        /// Write all messages (warnings) to end of text file
        for (const QString &msg : const_cast<const QStringList &>(msgList)) {
            static const char linebreak = '\n';
            f.write(&linebreak, 1);
            f.write(msg.toUtf8());
        }

        f.close();
    }
}
#endif // HAVE_POPPLERQT5
