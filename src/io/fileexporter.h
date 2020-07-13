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

#ifndef KBIBTEX_IO_FILEEXPORTER_H
#define KBIBTEX_IO_FILEEXPORTER_H

#include <QObject>

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

class QIODevice;

class File;
class Element;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT FileExporter : public QObject
{
    Q_OBJECT

public:
    enum class MessageSeverity {
        Info, ///< Messages that are of informative type
        Warning, ///< Messages that are of warning type
        Error ///< Messages that are of error type
    };

    explicit FileExporter(QObject *parent);
    ~FileExporter() override;

    /**
     * @brief Convert an element into a string representation.
     * @param[in] element Element to be converted into a string
     * @param[in] bibtexfile Bibliography which may provide additional insights; may be @c nullptr
     * @param[out] errorLog List of strings that receives error messages; may be @c nullptr
     * @return String representation of provided element
     */
    virtual QString toString(const QSharedPointer<const Element> element, const File *bibtexfile);

    /**
     * @brief Convert a bibliography into a string representation.
     * @param[in] bibtexfile Bibliography to be converted into a string
     * @param[out] errorLog List of strings that receives error messages; may be @c nullptr
     * @return String representation of provided bibliography
     */
    virtual QString toString(const File *bibtexfile);

    /**
     * Write a bibliography into a @c QIODevice like a file.
     * This function requires the @c iodevice to be open for write operations (@c QIODevice::WriteOnly).
     * The function will not close the @c iodevice upon return.
     * @param[in] element Bibliography to be written into the @c QIODevice
     * @param[out] errorLog List of strings that receives error messages; may be @c nullptr
     * @return @c true if writing the bibliography succeeded, else @c false
     */
    virtual bool save(QIODevice *iodevice, const File *bibtexfile) = 0;

    /**
     * Write an element into a @c QIODevice like a file.
     * This function requires the @c iodevice to be open for write operations (@c QIODevice::WriteOnly).
     * The function will not close the @c iodevice upon return.
     * @param[in] element Element to be written into the @c QIODevice
     * @param[in] bibtexfile Bibliography which may provide additional insights; may be @c nullptr
     * @param[out] errorLog List of strings that receives error messages; may be @c nullptr
     * @return @c true if writing the element succeeded, else @c false
     */
    virtual bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile) = 0;

    static QString numberToOrdinal(const int number);

signals:
    /**
     * @brief Signal to notify the caller of this class's functions about the progress.
     * @param[out] current Current progress, a non-negative number, less or equal @c total
     * @param[out] total Maximum value for progress, a non-negative number, greater or equal @c current
     */
    void progress(int current, int total);

    /**
     * Signal to notify the user of a FileExporter class about issues detected
     * during loading and parsing bibliographic data. Messages may be of various
     * severity levels. The message text may reveal additional information on
     * what the issue is and where it has been found (e.g. line number).
     * Implementations of FileExporter are recommended to print a similar message
     * as debug output.
     * TODO messages shall get i18n'ized if the code is compiled with/linked against
     * KDE Frameworks libraries.
     *
     * @param severity The message's severity level
     * @param messageText The message's text
     */
    void message(const FileExporter::MessageSeverity severity, const QString &messageText);

public slots:
    virtual void cancel() {
        // nothing
    }
};

Q_DECLARE_METATYPE(FileExporter::MessageSeverity)

#endif // KBIBTEX_IO_FILEEXPORTER_H
