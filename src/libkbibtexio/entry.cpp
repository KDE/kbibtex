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
#include <QList>

#include <entry.h>
#include <file.h>

#define max(a,b) ((a)>(b)?(a):(b))

using namespace KBibTeX::IO;

// FIXME: Check if using those constants in the program is really necessary
// or can be replace by config files
const QLatin1String Entry::ftAbstract = QLatin1String("abstract");
const QLatin1String Entry::ftAddress = QLatin1String("address");
const QLatin1String Entry::ftAuthor = QLatin1String("author");
const QLatin1String Entry::ftBookTitle = QLatin1String("booktitle");
const QLatin1String Entry::ftChapter = QLatin1String("chapter");
const QLatin1String Entry::ftCrossRef = QLatin1String("crossref");
const QLatin1String Entry::ftDOI = QLatin1String("doi");
const QLatin1String Entry::ftEditor = QLatin1String("editor");
const QLatin1String Entry::ftISSN = QLatin1String("issn");
const QLatin1String Entry::ftISBN = QLatin1String("isbn");
const QLatin1String Entry::ftJournal = QLatin1String("journal");
const QLatin1String Entry::ftKeywords = QLatin1String("Keywords");
const QLatin1String Entry::ftLocation = QLatin1String("location");
const QLatin1String Entry::ftMonth = QLatin1String("month");
const QLatin1String Entry::ftNote = QLatin1String("note");
const QLatin1String Entry::ftNumber = QLatin1String("number");
const QLatin1String Entry::ftPages = QLatin1String("pages");
const QLatin1String Entry::ftPublisher = QLatin1String("publisher");
const QLatin1String Entry::ftSeries = QLatin1String("series");
const QLatin1String Entry::ftTitle = QLatin1String("title");
const QLatin1String Entry::ftUrl = QLatin1String("url");
const QLatin1String Entry::ftVolume = QLatin1String("volume");
const QLatin1String Entry::ftYear = QLatin1String("year");

const QLatin1String Entry::etArticle = QLatin1String("article");
const QLatin1String Entry::etBook = QLatin1String("book");
const QLatin1String Entry::etInBook = QLatin1String("inbook");
const QLatin1String Entry::etInProceedings = QLatin1String("inproceedings");
const QLatin1String Entry::etMisc = QLatin1String("misc");
const QLatin1String Entry::etPhDThesis = QLatin1String("phdthesis");
const QLatin1String Entry::etTechReport = QLatin1String("techreport");

class Entry::EntryPrivate
{
public:
    QString m_type;
    QString m_id;
};

Entry::Entry(const QString& type, const QString& id)
        : Element(), QMap<QString, Value>(), d(new Entry::EntryPrivate)
{
    d->m_type = type;
    d->m_id = id;
}

Entry::Entry(const Entry &other)
        : Element(), QMap<QString, Value>(), d(new Entry::EntryPrivate)
{
    d->m_type = other.type();
    d->m_id = other.id();
    for (QMap<QString, Value>::ConstIterator it = other.begin(); it != other.end(); ++it)
        insert(it.key(), it.value());
}

Entry::~Entry()
{
    /*
    for (Fields::ConstIterator it = m_fields.begin(); it != m_fields.end(); it++) {
        delete(*it);
    }
    */
}

Entry& Entry::operator= (const Entry & other)
{
    if (this != &other) {
        d->m_type = other.type();
        d->m_id = other.id();
        clear();
        for (QMap<QString, Value>::ConstIterator it = other.begin(); it != other.end(); ++it)
            insert(it.key(), it.value());
    }
    return *this;
}


/*
Element* Entry::clone() const
{
    return new Entry(this);
}
*/

/* FIXME: Is this function required?
bool Entry::equals(const Entry &other)
{
    if (other.id().compare(id()) != 0)
        return false;

    for (Fields::ConstIterator it = m_fields.begin(); it != m_fields.end(); it++) {
        Field *field1 = *it;
        Field *field2 = other.getField(field1->keyName());

        if (field2 == NULL || field1->value() == NULL || field2->value() == NULL || field1->value()->text().compare(field2->value()->text()) != 0)
            return false;
    }

    return true;
}
*/

/* FIXME: Is this function required?
QString Entry::text() const
{
    QString result = "Id: ";
    result.append(m_id).append("  (").append(entryTypeString()).append(")\n");

    for (Fields::ConstIterator it = m_fields.begin(); it != m_fields.end(); it++) {
        result.append((*it)->keyName()).append(": ");
        result.append((*it)->value()->text()).append("\n");
    }

    return result;
}
*/

void Entry::setType(const QString& type)
{
    d->m_type = type;
}

QString Entry::type() const
{
    return d->m_type;
}

void Entry::setId(const QString& id)
{
    d->m_id = id;
}

QString Entry::id() const
{
    return d->m_id;
}

/*
bool Entry::containsPattern(const QString & pattern, Field::FieldType key, Element::FilterType filterType, Qt::CaseSensitivity caseSensitive) const
{
    if (filterType == ftExact) {
        ** check for exact match *
        bool result = key == Field::ftUnknown && m_id.contains(pattern, caseSensitive);

        for (Fields::ConstIterator it = m_fields.begin(); !result && it != m_fields.end(); it++)
            if (key == Field::ftUnknown || (*it) ->key() == key)
                result |= (*it) ->value().containsPattern(pattern, caseSensitive);

        return result;
    } else {
        ** for each word in the search pattern ... *
        QStringList words = pattern.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        bool *hits = new bool[words.count()];
        int i = 0;
        for (QStringList::ConstIterator wit = words.begin(); wit != words.end(); ++wit, ++i) {
            hits[i] = key == Field::ftUnknown && m_id.contains(*wit, caseSensitive);

            ** check if word is contained in any field *
            for (Fields::ConstIterator fit = m_fields.begin(); fit != m_fields.end(); ++fit)
                if (key == Field::ftUnknown || (*fit) ->key() == key)
                    hits[i] |= (*fit) ->value().containsPattern(*wit, caseSensitive);
        }

        int hitCount = 0;
        for (i = words.count() - 1; i >= 0; --i)
            if (hits[i]) ++hitCount;
        delete[] hits;

        ** return success depending on filter type and number of hits *
        return ((filterType == ftAnyWord && hitCount > 0) || (filterType == ftEveryWord && hitCount == words.count()));
    }
}
*/

/*
QStringList Entry::urls() const
{
    QStringList result;
    const QString fieldNames[] = {"localfile", "pdf", "ps", "postscript", "doi", "url", "howpublished", "ee", "biburl", "note"};
    const int fieldNamesCount = sizeof(fieldNames) / sizeof(fieldNames[0]);

    for (int j = 1; j < 5 ; ++j)   ** there may be variants such as url3 or doi2 *
        for (int i = 0; i < fieldNamesCount; i++) {
            Field * field = getField(fieldNames[ i ]);
            if ((field && !field->value().isEmpty())) {
                QString fieldName = fieldNames[i];
                ** field names should be like url, url2, url3, ... *
                if (j > 1) fieldName.append(QString::number(j));

                Field * field = getField(fieldName);
                if ((field && !field->value().isEmpty())) {
                    PlainText *plainText = dynamic_cast<PlainText*>(field->value().first());
                    if (plainText != NULL) {
                        QString plain = plainText->text();
                        int urlPos = plain.indexOf("\\url{", 0, Qt::CaseInsensitive);
                        if (urlPos > -1) {
                            plain = plain.mid(urlPos + 5);
                            urlPos = plain.indexOf("}", 0, Qt::CaseInsensitive);
                            if (urlPos > 0)
                                plain = plain.left(urlPos - 1);
                        }


                        if (fieldNames[ i ] == "doi" && !plain.startsWith("http", Qt::CaseInsensitive))
                            plain.prepend("http://dx.doi.org/");

                        result.append(plain);
                    }
                }
            }
        }

    return result;
}
*/

/*
bool Entry::addField(Field * field)
{
    m_fields.append(field);
    return TRUE;
}

Field* Entry::getField(const QString & key) const
{
    Field * result = NULL;

    for (Fields::ConstIterator it = m_fields.begin(); (it != m_fields.end()) && (result == NULL); it++)
        if ((*it) ->key().toLower() == key.toLower())
            result = *it;

    return result;
}

bool Entry::deleteField(const QString & key)
{
    for (Fields::Iterator it = m_fields.begin(); it != m_fields.end(); it++)
        if ((*it) ->key().toLower() == key.toLower()) {
            delete(*it);
            m_fields.erase(it);
            return TRUE;
        }

    return FALSE;
}

Entry::Fields::ConstIterator Entry::begin() const
{
    return m_fields.constBegin();
}

Entry::Fields::ConstIterator Entry::end() const
{
    return m_fields.constEnd();
}

int Entry::getFieldCount() const
{
    return m_fields.count();
}

void Entry::clearFields()
{
    for (Fields::ConstIterator it = m_fields.begin(); it != m_fields.end(); it++)
        delete(*it);
    m_fields.clear();
}
*/

/*
void Entry::copyFrom(const Entry *other)
{
    if (other == NULL) return;

    m_entryType = other->m_entryType;
    m_entryTypeString = other->m_entryTypeString;
    m_id = other->m_id;
    clearFields();
    for (Fields::ConstIterator it = other->m_fields.begin(); it != other->m_fields.end(); it++)
        m_fields.append(new Field(**it));
}
*/

/*
void Entry::merge(Entry *other, MergeSemantics mergeSemantics)
{
    for (Fields::ConstIterator it = other->m_fields.begin(); it != other->m_fields.end(); it++) {
        Field *otherField = new Field(**it);
        QString otherKey = otherField->key();
        Field *thisField = getField(otherKey);

        if (thisField == NULL) {
            m_fields.append(otherField);
        } else if (mergeSemantics == msForceAdding) {
            otherKey.prepend("OPT");
            otherField->setKey(otherKey);
            m_fields.append(otherField);
        }
    }
}
*/

/*
QString Entry::entryTypeToString(const EntryType entryType)
{
    switch (entryType) {
    case etArticle:
        return QString("Article");
    case etBook:
        return QString("Book");
    case etBooklet:
        return QString("Booklet");
    case etCollection:
        return QString("Collection");
    case etElectronic:
        return QString("Electronic");
    case etInBook:
        return QString("InBook");
    case etInCollection:
        return QString("InCollection");
    case etInProceedings:
        return QString("InProceedings");
    case etManual:
        return QString("Manual");
    case etMastersThesis:
        return QString("MastersThesis");
    case etMisc:
        return QString("Misc");
    case etPhDThesis:
        return QString("PhDThesis");
    case etProceedings:
        return QString("Proceedings");
    case etTechReport:
        return QString("TechReport");
    case etUnpublished:
        return QString("Unpublished");
    default:
        return QString("Unknown");
    }
}

Entry::EntryType Entry::entryTypeFromString(const QString & entryTypeString)
{
    QString entryTypeStringLower = entryTypeString.toLower();
    if (entryTypeStringLower == "article")
        return etArticle;
    else if (entryTypeStringLower == "book")
        return etBook;
    else if (entryTypeStringLower == "booklet")
        return etBooklet;
    else if (entryTypeStringLower == "collection")
        return etCollection;
    else if ((entryTypeStringLower == "electronic") || (entryTypeStringLower == "online") || (entryTypeStringLower == "internet") || (entryTypeStringLower == "webpage"))
        return etElectronic;
    else if (entryTypeStringLower == "inbook")
        return etInBook;
    else if (entryTypeStringLower == "incollection")
        return etInCollection;
    else if ((entryTypeStringLower == "inproceedings") || (entryTypeStringLower == "conference"))
        return etInProceedings;
    else if (entryTypeStringLower == "manual")
        return etManual;
    else if (entryTypeStringLower == "mastersthesis")
        return etMastersThesis;
    else if (entryTypeStringLower == "misc")
        return etMisc;
    else if (entryTypeStringLower == "phdthesis")
        return etPhDThesis;
    else if (entryTypeStringLower == "proceedings")
        return etProceedings;
    else if (entryTypeStringLower == "techreport")
        return etTechReport;
    else if (entryTypeStringLower == "unpublished")
        return etUnpublished;
    else
        return etUnknown;
}
*/

/*
Entry::FieldRequireStatus Entry::getRequireStatus(Entry::EntryType *entryType*, const QString& *key*)
{
// FIXME: This should be configured in an external file
    return Entry::frsIgnored; // dummy return
}
*/

