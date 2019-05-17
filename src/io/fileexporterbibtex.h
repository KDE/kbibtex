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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KBIBTEX_IO_FILEEXPORTERBIBTEX_H
#define KBIBTEX_IO_FILEEXPORTERBIBTEX_H

#include <QTextStream>

#include <Element>
#include <Value>
#include <FileExporter>

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

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
    Q_OBJECT

public:
    enum UseLaTeXEncoding {leUTF8, leLaTeX, leRaw};

    explicit FileExporterBibTeX(QObject *parent);
    ~FileExporterBibTeX() override;

    void setEncoding(const QString &encoding);

    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = nullptr) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = nullptr) override;

    static QString valueToBibTeX(const Value &value, const QString &fieldType = QString(), UseLaTeXEncoding useLaTeXEncoding = leLaTeX);

    /**
     * Cheap and fast test if another FileExporter is a FileExporterBibTeX object.
     * @param other another FileExporter object to test
     * @return true if FileExporter is actually a FileExporterBibTeX
     */
    static bool isFileExporterBibTeX(const FileExporter &other);

public slots:
    void cancel() override;

private:
    class FileExporterBibTeXPrivate;
    FileExporterBibTeXPrivate *d;

    inline QString applyEncoder(const QString &input, UseLaTeXEncoding useLaTeXEncoding) const;
    QString internalValueToBibTeX(const Value &value, const QString &fieldType = QString(), UseLaTeXEncoding useLaTeXEncoding = leLaTeX);

    static FileExporterBibTeX *staticFileExporterBibTeX;
};

#endif // KBIBTEX_IO_FILEEXPORTERBIBTEX_H
