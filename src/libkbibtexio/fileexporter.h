/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include "file.h"

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
    static const QString keyPaperSize;
    static const QString defaultPaperSize;

    static const QString keyFont;
    static const QString defaultFont;

    FileExporter();
    ~FileExporter();

    QString toString(const QSharedPointer<const Element> element);
    QString toString(const File *bibtexfile);

    virtual bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = NULL) = 0;
    virtual bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, QStringList *errorLog = NULL) = 0;

signals:
    void progress(int current, int total);

public slots:
    virtual void cancel() {
        // nothing
    };
};

#endif // KBIBTEX_IO_FILEEXPORTER_H
