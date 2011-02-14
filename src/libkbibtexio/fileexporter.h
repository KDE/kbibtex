/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#ifndef KBIBTEX_IO_FILEEXPORTER_H
#define KBIBTEX_IO_FILEEXPORTER_H

#include <QObject>
// #include <QMutex> // FIXME: required?

#include <file.h>

class QIODevice;

class File;
class Element;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class FileExporter : public QObject
{
    Q_OBJECT

public:
    FileExporter();
    ~FileExporter();

    virtual bool save(QIODevice *iodevice, const File* bibtexfile, QStringList *errorLog = NULL) = 0;
    virtual bool save(QIODevice *iodevice, const Element* element, QStringList *errorLog = NULL) = 0;

    /**
      * When exporting data, show a dialog where the user may select options on the
      * export process such as selecting encoding. Re-implementing this function is
      * optional and should only be done if user interaction is necessary at export
      * actions.
      * Return true if the configuration step was successful and the application
      * may proceed. If returned false, the export process has to be stopped.
      * The exporter may store configurations done here for future use (e.g. set default
      * values based on user input).
      * A calling application should call this function before calling save() or similar
      * functions. However, an application may opt to not call this function, e.g. in case
      * of a Save operation opposed to a SaveAs option which would call this function.
      * The implementing class must be aware of this behaviour.
      * The implementer may choose to show or not show a dialog, depending on e.g. if
      * additional information is necessary or not.
      */
    virtual bool showExportDialog(QWidget *parent, File *bibtexfile) {
        Q_UNUSED(parent);
        Q_UNUSED(bibtexfile);
        return true;
    }

signals:
    void progress(int current, int total);

public slots:
    virtual void cancel() {
        // nothing
    };

protected:
    // QMutex m_mutex; // FIXME: required?
};

#endif // KBIBTEX_IO_FILEEXPORTER_H
