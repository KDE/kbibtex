/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_IO_FILEIMPORTER_H
#define KBIBTEX_IO_FILEIMPORTER_H

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

#include <QObject>

class QIODevice;

class File;
class Person;

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileImporter : public QObject
{
    Q_OBJECT

public:
    enum MessageSeverity {
        SeverityInfo, ///< Messages that are of informative type, such as additional comma for last key-value pair in BibTeX entry
        SeverityWarning, ///< Messages that are of warning type, such as automatic corrections of BibTeX code without loss of information
        SeverityError ///< Messages that are of error type, which point to issue where information may get lost, e.g. invalid syntax or incomplete data
    };

    explicit FileImporter(QObject *parent);
    ~FileImporter() override;

    File *fromString(const QString &text);
    virtual File *load(QIODevice *iodevice) = 0;

    /**
      * When importing data, show a dialog where the user may select options on the
      * import process such as selecting encoding. Re-implementing this function is
      * optional and should only be done if user interaction is necessary at import
      * actions.
      * Return true if the configuration step was successful and the application
      * may proceed. If returned false, the import process has to be stopped.
      * The importer may store configurations done here for future use (e.g. set default
      * values based on user input).
      * A calling application should call this function before calling load() or similar
      * functions.
      * The implementer may choose to show or not show a dialog, depending on e.g. if
      * additional information is necessary or not.
      */
    virtual bool showImportDialog(QWidget *parent) {
        Q_UNUSED(parent);
        return true;
    }

    static bool guessCanDecode(const QString &) {
        return false;
    }

    /**
      * Split a person's name into its parts and construct a Person object from them.
      * This is a rather general functions and takes e.g. the curly brackets used in
      * (La)TeX not into account.
      * @param name The persons name
      * @return A Person object containing the name
      * @see Person
      */
    static Person *splitName(const QString &name);

private:
    static bool looksLikeSuffix(const QString &suffix);

signals:
    void progress(int current, int total);

    /**
     * Signal to notify the user of a FileImporter class about issues detected
     * during loading and parsing bibliographic data. Messages may be of various
     * severity levels. The message text may reveal additional information on
     * what the issue is and where it has been found (e.g. line number).
     * Implementations of FileImporter are recommended to print a similar message
     * as debug output.
     * TODO messages shall get i18n'ized if the code is compiled with/linked against
     * KDE Frameworks libraries.
     *
     * @param severity The message's severity level
     * @param messageText The message's text
     */
    void message(const FileImporter::MessageSeverity severity, const QString &messageText);

public slots:
    virtual void cancel() {
        // nothing
    }
};

Q_DECLARE_METATYPE(FileImporter::MessageSeverity)

#endif // KBIBTEX_IO_FILEIMPORTER_H
