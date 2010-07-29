/***************************************************************************
*   Copyright (C) 2004-2006 by Thomas Fischer                             *
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
#ifndef KBIBTEX_IO_FILEIMPORTER_H
#define KBIBTEX_IO_FILEIMPORTER_H

#include "kbibtexio_export.h"

#include <QObject>
#include <QMutex>

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
    FileImporter();
    ~FileImporter();

    File* fromString(const QString& text);
    virtual File* load(QIODevice *iodevice) = 0;

    static bool guessCanDecode(const QString &) {
        return FALSE;
    };

    /**
      * Split a person's name into its parts and construct a Person object from them.
      * This is a rather general functions and takes e.g. the curly brackets used in
      * (La)TeX not into account.
      * @param name The persons name
      * @return A Person object containing the name
      * @see Person
      */
    static Person* splitName(const QString& name);

signals:
    void parseError(int errorId);
    void progress(int current, int total);

public slots:
    virtual void cancel() {
        // nothing
    };

protected:
    QMutex m_mutex;
};

#endif // KBIBTEX_IO_FILEIMPORTER_H
