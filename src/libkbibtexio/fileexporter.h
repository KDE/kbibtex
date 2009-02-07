/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#include <QMutex>

#include <file.h>

class QIODevice;

namespace KBibTeX
{
namespace IO {

class File;
class Element;

/**
@author Thomas Fischer
*/
class FileExporter : public QObject
{
    Q_OBJECT

public:
    FileExporter();
    ~FileExporter();

    virtual bool save(QIODevice *iodevice, const File* bibtexfile, QStringList *errorLog = NULL) = 0;
    virtual bool save(QIODevice *iodevice, const Element* element, QStringList *errorLog = NULL) = 0;

signals:
    void progress(int current, int total);

public slots:
    virtual void cancel() {
        // nothing
    };

protected:
    QMutex m_mutex;
};

}
}

#endif // KBIBTEX_IO_FILEEXPORTER_H
