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

#include "entry.h"

#include <typeinfo>

#include <QRegularExpression>

#include "file.h"
#include "logging_data.h"

// FIXME: Check if using those constants in the program is really necessary
// or can be replace by config files
const QString Entry::ftAbstract = QStringLiteral("abstract");
const QString Entry::ftAddress = QStringLiteral("address");
const QString Entry::ftAuthor = QStringLiteral("author");
const QString Entry::ftBookTitle = QStringLiteral("booktitle");
const QString Entry::ftChapter = QStringLiteral("chapter");
const QString Entry::ftColor = QStringLiteral("x-color");
const QString Entry::ftComment = QStringLiteral("comment");
const QString Entry::ftCrossRef = QStringLiteral("crossref");
const QString Entry::ftDOI = QStringLiteral("doi");
const QString Entry::ftEdition = QStringLiteral("edition");
const QString Entry::ftEditor = QStringLiteral("editor");
const QString Entry::ftFile = QStringLiteral("file");
const QString Entry::ftISSN = QStringLiteral("issn");
const QString Entry::ftISBN = QStringLiteral("isbn");
const QString Entry::ftJournal = QStringLiteral("journal");
const QString Entry::ftKeywords = QStringLiteral("keywords");
const QString Entry::ftLocalFile = QStringLiteral("localfile");
const QString Entry::ftLocation = QStringLiteral("location");
const QString Entry::ftMonth = QStringLiteral("month");
const QString Entry::ftNote = QStringLiteral("note");
const QString Entry::ftNumber = QStringLiteral("number");
const QString Entry::ftPages = QStringLiteral("pages");
const QString Entry::ftPublisher = QStringLiteral("publisher");
const QString Entry::ftSchool = QStringLiteral("school");
const QString Entry::ftSeries = QStringLiteral("series");
const QString Entry::ftStarRating = QStringLiteral("x-stars");
const QString Entry::ftTitle = QStringLiteral("title");
const QString Entry::ftUrl = QStringLiteral("url");
const QString Entry::ftUrlDate = QStringLiteral("urldate");
const QString Entry::ftVolume = QStringLiteral("volume");
const QString Entry::ftYear = QStringLiteral("year");

const QString Entry::ftXData = QStringLiteral("xdata");

const QString Entry::etArticle = QStringLiteral("article");
const QString Entry::etBook = QStringLiteral("book");
const QString Entry::etInBook = QStringLiteral("inbook");
const QString Entry::etInProceedings = QStringLiteral("inproceedings");
const QString Entry::etProceedings = QStringLiteral("proceedings");
const QString Entry::etMisc = QStringLiteral("misc");
const QString Entry::etPhDThesis = QStringLiteral("phdthesis");
const QString Entry::etMastersThesis = QStringLiteral("mastersthesis");
const QString Entry::etTechReport = QStringLiteral("techreport");
const QString Entry::etUnpublished = QStringLiteral("unpublished");

quint64 Entry::internalUniqueIdCounter = 0;

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class Entry::EntryPrivate
{
public:
    QString type;
    QString id;
};

Entry::Entry(const QString &type, const QString &id)
        : Element(), QMap<QString, Value>(), internalUniqueId(++internalUniqueIdCounter), d(new Entry::EntryPrivate)
{
    d->type = type;
    d->id = id;
}

Entry::Entry(const Entry &other)
        : Element(), QMap<QString, Value>(), internalUniqueId(++internalUniqueIdCounter), d(new Entry::EntryPrivate)
{
    operator=(other);
}

Entry::~Entry()
{
    clear();
    delete d;
}

quint64 Entry::uniqueId() const
{
    return internalUniqueId;
}

bool Entry::operator==(const Entry &other) const
{
    /// Quick and easy tests first: id, type, and numer of fields
    if (id() != other.id() || type().compare(other.type(), Qt::CaseInsensitive) != 0 || count() != other.count())
        return false;

    /// Compare each field with other's corresponding field
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it) {
        if (!other.contains(it.key())) return false;
        const Value &thisValue = it.value();
        const Value &otherValue = other.value(it.key());
        if (thisValue != otherValue) return false;
    }

    /// All fields of the other entry must occurr as well in this entry
    /// (no need to check equivalence again)
    for (Entry::ConstIterator it = other.constBegin(); it != other.constEnd(); ++it)
        if (!contains(it.key())) return false;

    return true;
}

bool Entry::operator!=(const Entry &other) const
{
    return !operator ==(other);
}

Entry &Entry::operator= (const Entry &other)
{
    if (this != &other) {
        d->type = other.type();
        d->id = other.id();
        clear();
        for (Entry::ConstIterator it = other.constBegin(); it != other.constEnd(); ++it)
            insert(it.key(), it.value());
    }
    return *this;
}

void Entry::setType(const QString &type)
{
    d->type = type;
}

QString Entry::type() const
{
    return d->type;
}

void Entry::setId(const QString &id)
{
    d->id = id;
}

QString Entry::id() const
{
    return d->id;
}

const Value Entry::value(const QString &key) const
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return QMap<QString, Value>::value(it.key());

    return Value();
}

int Entry::remove(const QString &key)
{
    const QString lcKey = key.toLower();
    for (Entry::Iterator it = begin(); it != end(); ++it)
        if (it.key().toLower() == lcKey) {
            QMap<QString, Value>::erase(it);
            return 1;
        }

    return 0;
}

bool Entry::contains(const QString &key) const
{
    const QString lcKey = key.toLower();
    for (Entry::ConstIterator it = constBegin(); it != constEnd(); ++it)
        if (it.key().toLower() == lcKey)
            return true;

    return false;
}

Entry *Entry::resolveCrossref(const File *bibTeXfile) const
{
    Entry *result = new Entry(*this);

    if (bibTeXfile == nullptr)
        return result;

    static const QStringList crossRefFields = {ftCrossRef, ftXData};
    for (const QString &crossRefField : crossRefFields) {
        const QString crossRefValue = PlainTextValue::text(result->value(crossRefField));
        if (crossRefValue.isEmpty())
            continue;

        const QSharedPointer<Entry> crossRefEntry = bibTeXfile->containsKey(crossRefField, File::ElementType::Entry).dynamicCast<Entry>();
        if (!crossRefEntry.isNull()) {
            /// Copy all fields from crossref'ed entry to new entry which do not (yet) exist in the new entry
            for (Entry::ConstIterator it = crossRefEntry->constBegin(); it != crossRefEntry->constEnd(); ++it)
                if (!result->contains(it.key()))
                    result->insert(it.key(), Value(it.value()));

            if (crossRefEntry->type().compare(etProceedings, Qt::CaseInsensitive) && result->type().compare(etInProceedings, Qt::CaseInsensitive) && crossRefEntry->contains(ftTitle) && !result->contains(ftBookTitle)) {
                /// In case current entry is of type 'inproceedings' but lacks a 'book title'
                /// and the crossref'ed entry is of type 'proceedings' and has a 'title', then
                /// copy this 'title into as the 'book title' of the current entry.
                /// Note: the correct way should be that the crossref'ed entry has a 'book title'
                /// field, but that case was handled above when copying non-existing fields,
                /// so this if-block is only a fall-back case.
                result->insert(ftBookTitle, Value(crossRefEntry->operator [](ftTitle)));
            }

            /// Remove crossref field (no longer of use as all data got copied)
            result->remove(crossRefField);
        }
    }

    return result;
}

QStringList Entry::authorsLastName(const Entry &entry)
{
    Value value;
    if (entry.contains(Entry::ftAuthor))
        value = entry.value(Entry::ftAuthor);
    if (value.isEmpty() && entry.contains(Entry::ftEditor))
        value = entry.value(Entry::ftEditor);
    if (value.isEmpty())
        return QStringList();

    QStringList result;
    int maxAuthors = 16; ///< limit the number of authors considered
    result.reserve(maxAuthors);
    for (const QSharedPointer<ValueItem> &item : const_cast<const Value &>(value)) {
        QSharedPointer<const Person> person = item.dynamicCast<const Person>();
        if (!person.isNull()) {
            const QString lastName = person->lastName();
            if (!lastName.isEmpty())
                result << lastName;
        }
        if (--maxAuthors <= 0) break;   ///< limit the number of authors considered
    }

    return result;
}

QStringList Entry::authorsLastName() const
{
    return authorsLastName(*this);
}

int Entry::editionStringToNumber(const QString &editionString, bool *ok)
{
    // TODO   This function is not really used (much) yet. It can clean up
    // several types of non-standard variations used to describe the edition
    // of a publication, but the question is whether this re-write should be
    // done without the user's approval.
    // Thus, the intention is to make use of this function in a future 'clean
    // BibTeX file' feature which is to be triggered by the user and which
    // would rewrite and update a bibliography file. Ideas would include not
    // just rewriting the edition, but, for example updating URLs from HTTP
    // to HTTPS, retrieving new or updated data, etc.

    *ok = true; // Assume the best for now as this function only returns if successful (except for last return)

    // Test if string is just digits that can be converted into a positive int
    bool toIntok = false;
    int edition = editionString.toInt(&toIntok);
    if (toIntok && edition >= 1)
        return edition;

    const QString editionStringLower = editionString.toLower().trimmed();

    // Test if string starts with digits, followed by English ordinal suffices
    static const QRegularExpression englishOrdinal(QStringLiteral("^([1-9][0-9]*)(st|nd|rd|th)($| edition)"));
    const QRegularExpressionMatch englishOrdinalMatch = englishOrdinal.match(editionStringLower);
    if (englishOrdinalMatch.hasMatch()) {
        bool toIntok = false;
        int edition = englishOrdinalMatch.captured(1).toInt(&toIntok);
        if (toIntok && edition >= 1)
            return edition;
    }

    // Test if string is a spelled-out English ordinal (in some cases consider mis-spellings)
    if (editionStringLower == QLatin1String("first"))
        return 1;
    else if (editionStringLower == QLatin1String("second"))
        return 2;
    else if (editionStringLower == QLatin1String("third"))
        return 3;
    else if (editionStringLower == QLatin1String("fourth"))
        return 4;
    else if (editionStringLower == QLatin1String("fifth") || editionStringLower == QLatin1String("fivth"))
        return 5;
    else if (editionStringLower == QLatin1String("sixth"))
        return 6;
    else if (editionStringLower == QLatin1String("seventh"))
        return 7;
    else if (editionStringLower == QLatin1String("eighth") || editionStringLower == QLatin1String("eigth"))
        return 8;
    else if (editionStringLower == QLatin1String("nineth") || editionStringLower == QLatin1String("ninth"))
        return 9;
    else if (editionStringLower == QLatin1String("tenth"))
        return 10;
    else if (editionStringLower == QLatin1String("eleventh"))
        return 11;
    else if (editionStringLower == QLatin1String("twelvth") || editionStringLower == QLatin1String("twelfth"))
        return 12;
    else if (editionStringLower == QLatin1String("thirdteeth"))
        return 13;
    else if (editionStringLower == QLatin1String("fourteenth"))
        return 14;
    else if (editionStringLower == QLatin1String("fifteenth"))
        return 15;
    else if (editionStringLower == QLatin1String("sixteenth"))
        return 16;

    // No test above succeeded, so communicate that conversion failed
    *ok = false;
    return 0;
}

QString Entry::editionNumberToString(const int edition, const Preferences::BibliographySystem bibliographySystem)
{
    if (edition <= 0) {
        qCWarning(LOG_KBIBTEX_DATA) << "Cannot convert a non-positive number (" << edition << ") into a textual representation";
        return QString();
    }

    // According to http://mirrors.ctan.org/biblio/bibtex/contrib/doc/btxFAQ.pdf,
    // edition values should look like this:
    //  - for first to fifth, write "First" to "Fifth"
    //  - starting from sixth, use numeric form like "17th"
    // According to http://mirrors.ctan.org/macros/latex/contrib/biblatex/doc/biblatex.pdf,
    // edition values should by just numbers (digits) without text,
    // such as '1' in a @sa PlainText.

    if (bibliographySystem == Preferences::BibliographySystem::BibLaTeX)
        return QString::number(edition);

    // BibLaTeX was simple, now it becomes complex for BibTeX

    if (edition == 1)
        return QStringLiteral("First");
    else if (edition == 2)
        return QStringLiteral("Second");
    else if (edition == 3)
        return QStringLiteral("Third");
    else if (edition == 4)
        return QStringLiteral("Fourth");
    else if (edition == 5)
        return QStringLiteral("Fifth");
    else if (edition >= 20 && edition % 10 == 1) {
        // 21, 31, 41, ...
        return QString(QStringLiteral("%1st")).arg(edition);
    } else if (edition >= 20 && edition % 10 == 2) {
        // 22, 32, 42, ...
        return QString(QStringLiteral("%1nd")).arg(edition);
    } else if (edition >= 20 && edition % 10 == 3) {
        // 23, 33, 43, ...
        return QString(QStringLiteral("%1rd")).arg(edition);
    } else if (edition >= 6) {
        // Remaining editions: 6, 7, ..., 19, 20, 24, 25, ...
        return QString(QStringLiteral("%1th")).arg(edition);
    } else {
        // Unsupported editions, like -23
        qCWarning(LOG_KBIBTEX_DATA) << "Don't know how to convert number" << edition << "into an ordinal string for edition";
    }
    return QString();
}

bool Entry::isEntry(const Element &other) {
    return typeid(other) == typeid(Entry);
}

QDebug operator<<(QDebug dbg, const Entry &entry) {
    dbg.nospace() << "Entry " << entry.id() << " (uniqueId=" << entry.uniqueId() << "), has " << entry.count() << " key-value pairs";
    return dbg;
}
