/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
    explicit FileExporter(QObject *parent);
    ~FileExporter() override;

    QString toString(const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = nullptr);
    QString toString(const File *bibtexfile, QStringList *errorLog = nullptr);

    virtual bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = nullptr) = 0;
    virtual bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = nullptr) = 0;

signals:
    void progress(int current, int total);

public slots:
    virtual void cancel() {
        // nothing
    }
};

#endif // KBIBTEX_IO_FILEEXPORTER_H
