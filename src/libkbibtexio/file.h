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
#ifndef KBIBTEX_IO_FILE_H
#define KBIBTEX_IO_FILE_H

#include <QLinkedList>
#include <QMap>

#include <entryfield.h>
#include <element.h>

#include "kbibtexio_export.h"

class QDir;
class QString;
class QChar;
class QStringList;
class QWaitCondition;
class QProcess;
class QIODevice;

namespace KBibTeX
{
namespace IO {

class Element;
class Entry;
class String;
class FileExporter;

static const QString Months[] = {
    QString("January"), QString("February"), QString("March"), QString("April"), QString("May"), QString("June"), QString("July"), QString("August"), QString("September"), QString("October"), QString("November"), QString("December")
};

static const QString MonthsTriple[] = {
    QString("jan"), QString("feb"), QString("mar"), QString("apr"), QString("may"), QString("jun"), QString("jul"), QString("aug"), QString("sep"), QString("oct"), QString("nov"), QString("dec")
};

class KBIBTEXIO_EXPORT File : public QList<KBibTeX::IO::Element*>
{
public:

    File();
    virtual ~File();

    void append(KBibTeX::IO::Element* element);
    void append(File* other);
    void erase(KBibTeX::IO::Element* element);

    const Element *containsKey(const QString &key) const;
    QStringList allKeys() const;
    QString text() const;

    QStringList getAllValuesAsStringList(const EntryField::FieldType fieldType) const;
    QMap<QString, int> getAllValuesAsStringListWithCount(const EntryField::FieldType fieldType) const;
    void replaceValue(const QString& oldText, const QString& newText, const EntryField::FieldType fieldType);

    QString fileName;

private:
    void clearElements();

};

}
}

#endif // KBIBTEX_IO_FILE_H
