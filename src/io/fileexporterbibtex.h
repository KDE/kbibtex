/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <Preferences>
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

#include "encoder.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileExporterBibTeX : public FileExporter
{
    Q_OBJECT

public:

    explicit FileExporterBibTeX(QObject *parent);
    ~FileExporterBibTeX() override;

    /**
     * Set the text encoding used in the resulting bibliography file.
     * Example values for encoding include 'UTF-8' or 'ISO-8859-1'.
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

    QString toString(const QSharedPointer<const Element> element, const File *bibtexfile) override;
    QString toString(const File *bibtexfile) override;

    bool save(QIODevice *iodevice, const File *bibtexfile) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile) override;

    /**
     * Write a Value object into a BibTeX or BibLaTeX strign representation.
     * The transcription may follow field type-specific rules. The generated string
     * may keep non-ASCII characters or have them rewritten into a LaTeX representation.
     * @param[in] value Value to be written out
     * @param[in] fieldType optionally specified field type, e.g. @c url to enable field type-specific rules
     * @param[in] useLaTeXEncoding optional parameter how to handle non-ASCII characters
     * @return string representation of the Value object
     */
    static QString valueToBibTeX(const Value &value, Encoder::TargetEncoding targetEncoding, const QString &fieldType = QString());

    /**
     * Convert a positive int into a textual representation.
     * If the requested bibliography system is BibTeX, conversion follows BibTeX's
     * documentation: editions 1 to 5 get rewritten into text like 'fourth',
     * larger editions are numbers followed by an English ordinal suffix,
     * resulting in, for example, '42nd'.
     * If the requested bibliography system is BibLaTeX, conversion follows this
     * system's documentation: edition numbers are simply converted into a string
     * for use in a @c PlainText.
     * If conversion fails, e.g. for negative values of @c edition, an empty
     * string is returned.
     * @param[in] edition edition as positive number
     * @return Textual representation of the ordinal value or empty string if conversion failed
     */
    static QString editionNumberToString(const int edition, const Preferences::BibliographySystem bibliographySystem = Preferences::instance().bibliographySystem());

    /**
     * Cheap and fast test if another FileExporter is a FileExporterBibTeX object.
     * @param other another FileExporter object to test against
     * @return @c true if FileExporter is actually a FileExporterBibTeX, else @c false
     */
    static bool isFileExporterBibTeX(const FileExporter &other);

public slots:
    void cancel() override;

private:
    class Private;
    Private *d;
};

#endif // KBIBTEX_IO_FILEEXPORTERBIBTEX_H
