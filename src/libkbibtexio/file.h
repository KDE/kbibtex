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
#ifndef BIBTEXFILE_H
#define BIBTEXFILE_H

#include <QObject>
#include <QLinkedList>
#include <QMap>

#include <entryfield.h>

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

class File : public QObject
{
    Q_OBJECT

    friend class FileExporterXML;
    friend class FileExporterBibTeX;
    friend class FileExporterRIS;
    friend class FileExporter;
    friend class FileParser;

public:
    typedef QLinkedList<Element*> ElementList;

    enum FileFormat { formatUndefined = 0, formatBibTeX = 1, formatXML = 2, formatHTML = 3, formatPDF = 4, formatPS = 5, formatRTF = 6, formatRIS = 7, formatEndNote = 8, formatISI = 9 };
    enum Encoding {encImplicit = 0, encLaTeX = 1, encUTF8 = 2};
    enum StringProtection { spNone, spParanthesis, spQuote, spBoth };

    File();
    ~File();

    unsigned int count();
    /* FIXME: Port
            Element* at( const unsigned int index );
    */
    void append(const File *other, const Element *after = NULL);
    void appendElement(Element *element, const Element *after = NULL);
    void deleteElement(Element *element);
    static Element* cloneElement(Element *element);

    Element *containsKey(const QString &key);
    const Element *containsKeyConst(const QString &key) const;
    QStringList allKeys();
    QString text();

    ElementList::iterator begin();
    ElementList::iterator end();
    ElementList::const_iterator constBegin() const;
    ElementList::const_iterator constEnd() const;

    QStringList getAllValuesAsStringList(const EntryField::FieldType fieldType) const;
    QMap<QString, int> getAllValuesAsStringListWithCount(const EntryField::FieldType fieldType) const;
    void replaceValue(const QString& oldText, const QString& newText, const EntryField::FieldType fieldType);
    Entry *completeReferencedFieldsConst(const Entry *entry) const;
    void completeReferencedFields(Entry *entry) const;

    QString fileName;

protected:
    ElementList elements;

private:
    void clearElements();

};

}
}

#endif
