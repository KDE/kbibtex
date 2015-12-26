/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

// FIXME #include <poppler-qt4.h>

#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>

#include <KSharedConfig>
#include <KConfigGroup>

#include "kbibtex.h"
#include "entry.h"

static const QRegExp regExpEscapedChars = QRegExp("\\\\+([&_~])");

/// File types supported by "document preview"
static const QStringList documentFileExtensions = QStringList() << QStringLiteral(".pdf") << QStringLiteral(".pdf.gz") << QStringLiteral(".pdf.bz2") << QStringLiteral(".ps") << QStringLiteral(".ps.gz") << QStringLiteral(".ps.bz2") << QStringLiteral(".eps") << QStringLiteral(".eps.gz") << QStringLiteral(".eps.bz2") << QStringLiteral(".html") << QStringLiteral(".xhtml") << QStringLiteral(".htm") << QStringLiteral(".dvi") << QStringLiteral(".djvu") << QStringLiteral(".wwf") << QStringLiteral(".jpeg") << QStringLiteral(".jpg") << QStringLiteral(".png") << QStringLiteral(".gif") << QStringLiteral(".tif") << QStringLiteral(".tiff");

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
    static QMimeDatabase db;
    static const QString invalidExtension = QStringLiteral("XXXXXXXXXXXXXXXXXXXXXXX");
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
    if (extension == mimetypeBibTeXExt)
        return mtBibTeX;
    else if (extension == mimetypeRISExt)
        return mtRIS;
    else if (extension == mimetypePDFExt)
        return mtPDF;
    // TODO other extensions

    /// Let the KDE subsystem guess the mime type
    QMimeType result = db.mimeTypeForUrl(url);
    /// Fall back to application/octet-stream if something goes wrong
    if (!result.isValid())
        result = mtOctetStream;

    /// In case that KDE could not determine mime type,
    /// do some educated guesses on our own
    if (!result.isValid() || result.name() == mimetypeOctetStream) {
        if (url.scheme().startsWith(QStringLiteral("http")))
            result = mtHTML;
        // TODO more tests?
    }

    return result;
}

void FileInfo::urlsInText(const QString &text, TestExistence testExistence, const QString &baseDirectory, QList<QUrl> &result)
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
        QUrl url(doiUrlPrefix() + match.remove(QStringLiteral("\\")));
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
                const QUrl url = QUrl::fromLocalFile(fileInfo.canonicalFilePath());
                if (fileInfo.exists() && fileInfo.isFile() && url.isValid() && !result.contains(url)) {
                    result << url;
                    /// Stop searching for URLs or filenames in current internal text
                    continue;
                }
            } else {
                /// Either the filename fragment is an absolute path OR no base directory
                /// was given (current working directory is assumed), ...
                const QFileInfo fileInfo(internalText);
                const QUrl url = QUrl::fromLocalFile(fileInfo.canonicalFilePath());
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
            QUrl url(match);
            if (url.isValid() && (testExistence == TestExistenceNo || !url.isLocalFile() || QFileInfo(url.toLocalFile()).exists()) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// explicitly check URL entry, may be an URL even if http:// or alike is missing
        pos = 0;
        while ((pos = KBibTeX::domainNameRegExp.indexIn(internalText, pos)) > -1) {
            int pos2 = internalText.indexOf(QStringLiteral(" "), pos + 1);
            if (pos2 < 0) pos2 = internalText.length();
            QString match = internalText.mid(pos, pos2 - pos);
            QUrl url("http://" + match);
            if (url.isValid() && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// extract general file-like patterns
        pos = 0;
        while ((pos = KBibTeX::fileRegExp.indexIn(internalText, pos)) != -1) {
            QString match = KBibTeX::fileRegExp.cap(0);
            QUrl url(match);
            if (url.isValid() && (testExistence == TestExistenceNo || !url.isLocalFile() || QFileInfo(url.toLocalFile()).exists()) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }
    }
}

QList<QUrl> FileInfo::entryUrls(const Entry *entry, const QUrl &bibTeXUrl, TestExistence testExistence)
{
    QList<QUrl> result;
    if (entry == NULL || entry->isEmpty())
        return result;

    if (entry->contains(Entry::ftDOI)) {
        const QString doi = PlainTextValue::text(entry->value(Entry::ftDOI));
        if (!doi.isEmpty() && KBibTeX::doiRegExp.indexIn(doi) == 0) {
            QString match = KBibTeX::doiRegExp.cap(0);
            QUrl url(doiUrlPrefix() + match.remove(QStringLiteral("\\")));
            result.append(url);
        }
    }
    static const QString etPMID = QStringLiteral("pmid");
    if (entry->contains(etPMID)) {
        const QString pmid = PlainTextValue::text(entry->value(etPMID));
        bool ok = false;
        ok &= pmid.toInt(&ok) > 0;
        if (ok) {
            QUrl url(QStringLiteral("https://www.ncbi.nlm.nih.gov/pubmed/") + pmid);
            result.append(url);
        }
    }
    static const QString etEPrint = QStringLiteral("eprint");
    if (entry->contains(etEPrint)) {
        const QString eprint = PlainTextValue::text(entry->value(etEPrint));
        if (!eprint.isEmpty()) {
            QUrl url(QStringLiteral("http://arxiv.org/search?query=") + eprint);
            result.append(url);
        }
    }

    const QString baseDirectory = bibTeXUrl.isValid() ? bibTeXUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path() : QString();

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
        /// check if in the same directory as the BibTeX file
        /// a PDF file exists which filename is based on the entry's id
        for (QStringList::ConstIterator extensionIt = documentFileExtensions.constBegin(); extensionIt != documentFileExtensions.constEnd(); ++extensionIt) {
            const QFileInfo fi(baseDirectory + QDir::separator() + entry->id() + *extensionIt);
            if (fi.exists()) {
                const QUrl url = QUrl::fromLocalFile(fi.canonicalFilePath());
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
                const QUrl url = QUrl::fromLocalFile(fi.canonicalFilePath());
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
    QString textFilename = QString(pdfFilename).remove(invalidChars).append(QStringLiteral(".txt")).prepend(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QLatin1Char('/') + "pdftotext/");

    /// Initialize return value
    QString text;

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
    if (text.isEmpty()) {
        /// Load PDF file through Poppler
        // FIXME
        /*
        Poppler::Document *doc = Poppler::Document::load(pdfFilename);
        if (doc != NULL) {
            /// Build text by appending each page's text
            text = QStringLiteral("");
            for (int i = 0; i < doc->numPages(); ++i)
                text.append(doc->page(i)->text(QRect())).append(QStringLiteral("\n\n"));
            delete doc;

            /// Save text in cache file
            QFile f(textFilename);
            if (f.open(QFile::WriteOnly)) {
                QTextStream ts(&f);
                ts << text;
                f.close();
            }
        }
        */
    }

    return text;
}

QString FileInfo::doiUrlPrefix()
{
    KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
    static const QString configGroupNameNetworking(QStringLiteral("Networking"));
    static const QString keyDOIUrlPrefix(QStringLiteral("DOIUrlPrefix"));
    KConfigGroup configGroup(config, configGroupNameNetworking);
    return configGroup.readEntry(keyDOIUrlPrefix, KBibTeX::doiUrlPrefix);
}
