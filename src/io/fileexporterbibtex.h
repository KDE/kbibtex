/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
    enum class UseLaTeXEncoding {UTF8, LaTeX, Raw};

    explicit FileExporterBibTeX(QObject *parent);
    ~FileExporterBibTeX() override;

    /**
     * Set the text encoding used in the resulting bibliography file.
     * Example values for encoding include 'UTF-8' or 'US-ASCII'.
     * Special encoding 'LaTeX' corresponds to 'UTF-8' but tries to
     * replace non-ASCII characters with LaTeX equivalents (which are
     * ASCII only).
     * This setting both overrules the Preferences' global setting
     * as well as the encoding that may have been set in the bibliography's
     * properties (which may got set when loading the bibliography from
     * a file).
     *
     * @param[in] encoding encoding to be forced upon the output by this exporter
     */
    void setEncoding(const QString &encoding);

    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = nullptr) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = nullptr) override;

    /**
     * Write a Value object into a BibTeX or BibLaTeX strign representation.
     * The transcription may follow field type-specific rules. The generated string
     * may keep non-ASCII characters or have them rewritten into a LaTeX representation.
     * @param[in] value Value to be written out
     * @param[in] fieldType optionally specified field type, e.g. @c url to enable field type-specific rules
     * @param[in] useLaTeXEncoding optional parameter how to handle non-ASCII characters
     * @return string representation of the Value object
     */
    static QString valueToBibTeX(const Value &value, const QString &fieldType = QString(), UseLaTeXEncoding useLaTeXEncoding = UseLaTeXEncoding::LaTeX);

    /**
     * Cheap and fast test if another FileExporter is a FileExporterBibTeX object.
     * @param other another FileExporter object to test against
     * @return @c true if FileExporter is actually a FileExporterBibTeX, else @c false
     */
    static bool isFileExporterBibTeX(const FileExporter &other);

public slots:
    void cancel() override;

private:
    class FileExporterBibTeXPrivate;
    FileExporterBibTeXPrivate *d;

    inline QString applyEncoder(const QString &input, UseLaTeXEncoding useLaTeXEncoding) const;
    QString internalValueToBibTeX(const Value &value, const QString &fieldType = QString(), UseLaTeXEncoding useLaTeXEncoding = UseLaTeXEncoding::LaTeX);

    static FileExporterBibTeX *staticFileExporterBibTeX;
};

#endif // KBIBTEX_IO_FILEEXPORTERBIBTEX_H
