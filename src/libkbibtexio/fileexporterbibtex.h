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
#ifndef BIBTEXFILEEXPORTERBIBTEX_H
#define BIBTEXFILEEXPORTERBIBTEX_H

#include <QTextStream>

#include <kbibtexnamespace.h>
#include "element.h"
#include "fileexporter.h"
#include "value.h"

class QChar;

class Comment;
class Preamble;
class Macro;
class Entry;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileExporterBibTeX : public FileExporter
{
public:
    enum QuoteComment {qcNone, qcCommand, qcPercentSign};

    FileExporterBibTeX();
    ~FileExporterBibTeX();

    void setEncoding(const QString& encoding);
    void setStringDelimiters(const QChar& stringOpenDelimiter, const QChar& stringCloseDelimiter);
    void setKeywordCasing(KBibTeX::Casing keywordCasing);
    void setQuoteComment(QuoteComment quoteComment);
    void setProtectCasing(bool protectCasing);

    bool save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice* iodevice, const Element* element, QStringList *errorLog = NULL);

    static QString valueToBibTeX(const Value& value, const QString& fieldType = QString::null);
    static QString elementToString(const Element* element);

public slots:
    void cancel();

protected:
    static bool requiresPersonQuoting(const QString &text, bool isLastName);
    static QString escapeLaTeXChars(QString &text);

private:
    QChar m_stringOpenDelimiter;
    QChar m_stringCloseDelimiter;
    KBibTeX::Casing m_keywordCasing;
    QuoteComment m_quoteComment;
    QString m_encoding;
    bool m_protectCasing;
    bool cancelFlag;

    bool writeEntry(QTextStream &stream, const Entry& entry);
    bool writeMacro(QTextStream &stream, const Macro& macro);
    bool writeComment(QTextStream &stream, const Comment& comment);
    bool writePreamble(QTextStream &stream, const  Preamble& preamble);
    bool writeString(QTextStream &stream, const QString& text);

    void addProtectiveCasing(QString &text);
};

#endif
