/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#ifndef BIBTEXFILEEXPORTERBIBTEX_H
#define BIBTEXFILEEXPORTERBIBTEX_H

#include <QTextStream>

#include "kbibtexnamespace.h"
#include "element.h"
#include "value.h"
#include "fileexporter.h"

class QChar;

class Comment;
class Preamble;
class Macro;
class Entry;
class IConvLaTeX;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileExporterBibTeX : public FileExporter
{
public:
    enum UseLaTeXEncoding {leUTF8, leLaTeX, leRaw};

    FileExporterBibTeX();
    ~FileExporterBibTeX();

    void setEncoding(const QString &encoding);

    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = NULL);
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = NULL);

    static QString valueToBibTeX(const Value &value, const QString &fieldType = QString(), UseLaTeXEncoding useLaTeXEncoding = leLaTeX);

public slots:
    void cancel();

private:
    class FileExporterBibTeXPrivate;
    FileExporterBibTeXPrivate *d;

    QString internalValueToBibTeX(const Value &value, const QString &fieldType = QString(), UseLaTeXEncoding useLaTeXEncoding = leLaTeX);

    static FileExporterBibTeX *staticFileExporterBibTeX;
};

#endif