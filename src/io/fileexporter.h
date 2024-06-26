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

#ifndef KBIBTEX_IO_FILEEXPORTER_H
#define KBIBTEX_IO_FILEEXPORTER_H

#include <QObject>

#ifdef HAVE_KF
#include "kbibtexio_export.h"
#endif // HAVE_KF

class QIODevice;
class QFileInfo;

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

    static FileExporter *factory(const QFileInfo &fileInfo, const QString &exporterClassHint, QObject *parent);
    static FileExporter *factory(const QUrl &url, const QString &exporterClassHint, QObject *parent);
    static QVector<QString> exporterClasses(const QFileInfo &fileInfo);
    static QVector<QString> exporterClasses(const QUrl &url);

    /**
     * @brief Convert an element into a string representation.
     * @param[in] element Element to be converted into a string
     * @param[in] bibtexfile Bibliography which may provide additional insights; may be @c nullptr
     * @param[out] errorLog List of strings that receives error messages; may be @c nullptr
     * @return String representation of provided element
     */
    virtual QString toString(const QSharedPointer<const Element> &element, const File *bibtexfile);

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
    virtual bool save(QIODevice *iodevice, const QSharedPointer<const Element> &element, const File *bibtexfile) = 0;

    static QString numberToOrdinal(const int number, bool onlyText = true);

    /**
     * Convert a textual representation of an edition string into a number.
     * Examples for supported string patterns include '4', '4th', or 'fourth'.
     * Success of the conversion is returned via the @c ok variable, where the
     * function caller has to provide a pointer to a boolean variable.
     * In case of success, the function's result is the edition, in case
     * of failure, i.e. @c *ok==false, the result is undefined.
     * @param[in] editionString A string representing an edition number
     * @param[out] ok Pointer to a boolean variable used to return the success (@c true) or failure (@c false) state of the conversion; must not be @c nullptr
     * @return In case of success, the edition as a positive int, else undefined
     */
    static int editionStringToNumber(const QString &editionString, bool *ok);

    /**
     * Convert a textual representation of a month string into a number.
     * Examples for supported string patterns include 'January', 'jan', or '1'.
     * Success of the conversion is returned via the @c ok variable, where the
     * function caller has to provide a pointer to a boolean variable.
     * In case of success, the function's result is the month in the range
     * 1 (January) to 12 (December), in case of failure, i.e. @c *ok==false,
     * the result is undefined.
     * @param[in] monthString A string representing a month
     * @param[out] ok Pointer to a boolean variable used to return the success (@c true) or failure (@c false) state of the conversion; must not be @c nullptr
     * @return In case of success, the month as a positive int in the range 1 to 12, else undefined
     */
    static int monthStringToNumber(const QString &monthString, bool *ok = nullptr);

Q_SIGNALS:
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

public Q_SLOTS:
    virtual void cancel() {
        // nothing
    }
};

Q_DECLARE_METATYPE(FileExporter::MessageSeverity)

#endif // KBIBTEX_IO_FILEEXPORTER_H
