/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <QFileInfo>
#include <QDir>


#include <kbibtexnamespace.h>
#include <entry.h>
#include "fileinfo.h"

static const QRegExp regExpFileExtension = QRegExp("\\.[a-z0-9]{1,4}", Qt::CaseInsensitive);
static const QRegExp regExpEscapedChars = QRegExp("\\\\+([&_~])");
static const QStringList documentFileExtensions = QStringList() << ".pdf" << ".ps";

FileInfo::FileInfo()
{
    // TODO
}

void FileInfo::urlsInText(const QString &text, bool testExistance, const QString &baseDirectory, QList<KUrl> &result)
{
    if (text.isEmpty())
        return;

    QStringList fileList = text.split(KBibTeX::fileListSeparatorRegExp, QString::SkipEmptyParts);
    for (QStringList::ConstIterator filesIt = fileList.constBegin(); filesIt != fileList.constEnd(); ++filesIt) {
        QString internalText = *filesIt;

        if (testExistance) {
            QFileInfo fileInfo(internalText);
            KUrl url = KUrl(fileInfo.filePath());
            if (fileInfo.exists() && fileInfo.isFile() && !result.contains(url)) {
                /// text points to existing file (most likely with absolute path)
                result << url;
                /// stop searching for urls or filenames in current internal text
                continue;
            } else if (!baseDirectory.isEmpty()) {
                const QString fullFilename = baseDirectory + QDir::separator() + internalText;
                fileInfo = QFileInfo(fullFilename);
                url = KUrl(fileInfo.filePath());
                if (fileInfo.exists() && fileInfo.isFile() && !result.contains(url)) {
                    /// text points to existing file in base directory
                    result << url;
                    /// stop searching for urls or filenames in current internal text
                    continue;
                }
            }
        }

        /// extract URL from current field
        int pos = 0;
        while ((pos = KBibTeX::urlRegExp.indexIn(internalText, pos)) != -1) {
            QString match = KBibTeX::urlRegExp.cap(0);
            KUrl url(match);
            if (url.isValid() && (!testExistance || !url.isLocalFile() || QFileInfo(url.path()).exists()) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// extract DOI from current field
        pos = 0;
        while ((pos = KBibTeX::doiRegExp.indexIn(internalText, pos)) != -1) {
            QString match = KBibTeX::doiRegExp.cap(0);
            KUrl url(KBibTeX::doiUrlPrefix + match.replace("\\", ""));
            if (url.isValid() && !result.contains(url))
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
                if (!result.contains(url))
                    result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }

        /// extract general file-like patterns
        pos = 0;
        while ((pos = KBibTeX::fileRegExp.indexIn(internalText, pos)) != -1) {
            QString match = KBibTeX::fileRegExp.cap(0);
            KUrl url(match);
            if (url.isValid() && (!testExistance || !url.isLocalFile() || QFileInfo(url.pathOrUrl()).exists()) && !result.contains(url))
                result << url;
            /// remove match from internal text to avoid duplicates
            internalText = internalText.left(pos) + internalText.mid(pos + match.length());
        }
    }
}

QList<KUrl> FileInfo::entryUrls(const Entry *entry, const KUrl &bibTeXUrl)
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

            urlsInText(plainText, true, baseDirectory, result);
        }
    }

    if (!baseDirectory.isEmpty()) {
        /// check if in the same directory as the BibTeX file
        /// a PDF file exists which filename is based on the entry's id
        for (QStringList::ConstIterator extensionIt = documentFileExtensions.constBegin(); extensionIt != documentFileExtensions.constEnd(); ++extensionIt) {
            KUrl url(baseDirectory + QDir::separator() + entry->id() + *extensionIt);
            if (QFileInfo(url.path()).exists() && !result.contains(url))
                result << url;
        }

        /// check if in the same directory as the BibTeX file there is a subdirectory
        /// similar to the BibTeX file's name and which contains a PDF file exists
        /// which filename is based on the entry's id
        QString basename = bibTeXUrl.fileName().replace(QRegExp("\\.[^.]{2,5}$"), "");
        QString directory = baseDirectory + QDir::separator() + basename;
        for (QStringList::ConstIterator extensionIt = documentFileExtensions.constBegin(); extensionIt != documentFileExtensions.constEnd(); ++extensionIt) {
            KUrl url(directory + QDir::separator() + entry->id() + *extensionIt);
            if (QFileInfo(url.path()).exists() && !result.contains(url))
                result << url;
        }
    }

    return result;
}
