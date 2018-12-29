/***************************************************************************
 *   Copyright (C) 2016-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "bibliographymodel.h"

#include <QSettings>

#include "entry.h"
#include "onlinesearchabstract.h"

const int SortedBibliographyModel::SortAuthorNewestTitle = 0;
const int SortedBibliographyModel::SortAuthorOldestTitle = 1;
const int SortedBibliographyModel::SortNewestAuthorTitle = 2;
const int SortedBibliographyModel::SortOldestAuthorTitle = 3;

SortedBibliographyModel::SortedBibliographyModel()
        : QSortFilterProxyModel(), m_sortOrder(0), model(new BibliographyModel()) {
    const QSettings settings(QStringLiteral("harbour-bibsearch"), QStringLiteral("BibSearch"));
    m_sortOrder = settings.value(QStringLiteral("sortOrder"), 0).toInt();

    setSourceModel(model);
    setDynamicSortFilter(true);
    sort(0);
    connect(model, &BibliographyModel::busyChanged, this, &SortedBibliographyModel::busyChanged);
}

SortedBibliographyModel::~SortedBibliographyModel() {
    QSettings settings(QStringLiteral("harbour-bibsearch"), QStringLiteral("BibSearch"));
    settings.setValue(QStringLiteral("sortOrder"), m_sortOrder);

    delete model;
}

QHash<int, QByteArray> SortedBibliographyModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[BibTeXIdRole] = "bibtexid";
    roles[FoundViaRole] = "foundVia";
    roles[TitleRole] = "title";
    roles[AuthorRole] = "author";
    roles[AuthorShortRole] = "authorShort";
    roles[YearRole] = "year";
    roles[WherePublishedRole] = "wherePublished";
    roles[UrlRole] = "url";
    roles[DoiRole] = "doi";
    return roles;
}

bool SortedBibliographyModel::isBusy() const {
    if (model != nullptr)
        return model->isBusy();
    else
        return false;
}

int SortedBibliographyModel::sortOrder() const {
    return m_sortOrder;
}

void SortedBibliographyModel::setSortOrder(int _sortOrder) {
    if (_sortOrder != m_sortOrder) {
        m_sortOrder = _sortOrder;
        invalidate();
        emit sortOrderChanged(m_sortOrder);
    }
}

QStringList SortedBibliographyModel::humanReadableSortOrder() const {
    static const QStringList result {
                                     tr("Last name, newest first"), ///< SortAuthorNewestTitle
                                     tr("Last name, oldest first"), ///< SortAuthorOldestTitle
                                     tr("Newest first, last name"), ///< SortNewestAuthorTitle
                                     tr("Oldest first, last name")  ///< SortOldestAuthorTitle
                                    };
    return result;
}

void SortedBibliographyModel::startSearch(const QString &freeText, const QString &title, const QString &author) {
    if (model != nullptr)
        model->startSearch(freeText, title, author);
}

void SortedBibliographyModel::clear() {
    if (model != nullptr)
        model->clear();
}

bool SortedBibliographyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
    if (model == nullptr)
        return source_left.row() < source_right.row();

    const QSharedPointer<const Entry> entryLeft = model->entry(source_left.row());
    const QSharedPointer<const Entry> entryRight = model->entry(source_right.row());
    if (entryLeft.isNull() || entryRight.isNull())
        return source_left.row() < source_right.row();

    SortingTriState sortingTriState = Undecided;
    switch (m_sortOrder) {
    case SortAuthorNewestTitle:
        sortingTriState = compareTitles(entryLeft, entryRight, compareYears(entryLeft, entryRight, MostRecentFirst, compareAuthors(entryLeft, entryRight)));
        break;
    case SortAuthorOldestTitle:
        sortingTriState = compareTitles(entryLeft, entryRight, compareYears(entryLeft, entryRight, LeastRecentFirst, compareAuthors(entryLeft, entryRight)));
        break;
    case SortNewestAuthorTitle:
        sortingTriState = compareTitles(entryLeft, entryRight, compareAuthors(entryLeft, entryRight, compareYears(entryLeft, entryRight, MostRecentFirst)));
        break;
    case SortOldestAuthorTitle:
        sortingTriState = compareTitles(entryLeft, entryRight, compareAuthors(entryLeft, entryRight, compareYears(entryLeft, entryRight, LeastRecentFirst)));
        break;
    default:
        sortingTriState = Undecided;
    }

    switch (sortingTriState) {
    case True:
        return true;
    case False:
        return false;
    default:
        return (source_left.row() < source_right.row());
    }
}

SortedBibliographyModel::SortingTriState SortedBibliographyModel::compareAuthors(const QSharedPointer<const Entry> entryLeft, const QSharedPointer<const Entry> entryRight, SortedBibliographyModel::SortingTriState previousDecision) const {
    if (previousDecision != Undecided) return previousDecision;

    const Value authorsLeft = entryLeft->operator[](Entry::ftAuthor);
    const Value authorsRight = entryRight->operator[](Entry::ftAuthor);
    int p = 0;
    while (p < authorsLeft.count() && p < authorsRight.count()) {
        const QSharedPointer<Person> personLeft = authorsLeft.at(p).dynamicCast<Person>();
        const QSharedPointer<Person> personRight = authorsRight.at(p).dynamicCast<Person>();
        if (personLeft.isNull() || personRight.isNull())
            return Undecided;

        const int cmpLast = removeNobiliaryParticle(personLeft->lastName()).localeAwareCompare(removeNobiliaryParticle(personRight->lastName()));
        if (cmpLast < 0) return True;
        else if (cmpLast > 0) return False;
        const int cmpFirst = personLeft->firstName().left(1).localeAwareCompare(personRight->firstName().left(1));
        if (cmpFirst < 0) return True;
        else if (cmpFirst > 0) return False;

        ++p;
    }
    if (authorsLeft.count() < authorsRight.count()) return True;
    else if (authorsLeft.count() > authorsRight.count()) return False;

    return Undecided;
}

SortedBibliographyModel::SortingTriState SortedBibliographyModel::compareYears(const QSharedPointer<const Entry> entryLeft, const QSharedPointer<const Entry> entryRight, AgeSorting ageSorting, SortedBibliographyModel::SortingTriState previousDecision) const {
    if (previousDecision != Undecided) return previousDecision;

    bool yearLeftOk = false, yearRightOk = false;
    const int yearLeft = BibliographyModel::valueToText(entryLeft->operator[](Entry::ftYear)).toInt(&yearLeftOk);
    const int yearRight = BibliographyModel::valueToText(entryRight->operator[](Entry::ftYear)).toInt(&yearRightOk);
    if (yearLeftOk && yearRightOk && yearLeft > 1000 && yearRight > 1000 && yearLeft < 3000 && yearRight < 3000) {
        if (yearLeft < yearRight) return ageSorting == LeastRecentFirst ? True : False;
        else if (yearLeft > yearRight) return ageSorting == LeastRecentFirst ? False : True;
    }

    return Undecided;
}

SortedBibliographyModel::SortingTriState SortedBibliographyModel::compareTitles(const QSharedPointer<const Entry> entryLeft, const QSharedPointer<const Entry> entryRight, SortedBibliographyModel::SortingTriState previousDecision) const {
    if (previousDecision != Undecided) return previousDecision;

    const QString titleLeft = BibliographyModel::valueToText(entryLeft->operator[](Entry::ftTitle));
    const QString titleRight = BibliographyModel::valueToText(entryRight->operator[](Entry::ftTitle));
    const int titleCmp = titleLeft.localeAwareCompare(titleRight);
    if (titleCmp < 0) return True;
    else if (titleCmp > 0) return False;

    return Undecided;
}

QString SortedBibliographyModel::removeNobiliaryParticle(const QString &lastname) const {
    static const QStringList nobiliaryParticles {QStringLiteral("af "), QStringLiteral("d'"), QStringLiteral("de "), QStringLiteral("di "), QStringLiteral("du "), QStringLiteral("of "), QStringLiteral("van "), QStringLiteral("von "), QStringLiteral("zu ")};
    for (QStringList::ConstIterator it = nobiliaryParticles.constBegin(); it != nobiliaryParticles.constEnd(); ++it)
        if (lastname.startsWith(*it))
            return lastname.mid(it->length());
    return lastname;
}

BibliographyModel::BibliographyModel() {
    m_file = new File();
    m_runningSearches = 0;

    m_searchEngineList = new SearchEngineList();
    connect(m_searchEngineList, &SearchEngineList::foundEntry, this, &BibliographyModel::newEntry);
    connect(m_searchEngineList, &SearchEngineList::busyChanged, this, &BibliographyModel::busyChanged);
}

BibliographyModel::~BibliographyModel() {
    delete m_file;
    delete m_searchEngineList;
}

int BibliographyModel::rowCount(const QModelIndex &parent) const {
    if (parent == QModelIndex())
        return m_file->count();
    else
        return 0;
}

QVariant BibliographyModel::data(const QModelIndex &index, int role) const {
    if (index.row() < 0 || index.row() >= m_file->count() || index.column() != 0)
        return QVariant();

    const QSharedPointer<const Entry> curEntry = entry(index.row());

    if (!curEntry.isNull()) {
        QString fieldName;
        switch (role) {
        case Qt::DisplayRole: /// fall-through on purpose
        case TitleRole: fieldName = Entry::ftTitle; break;
        case AuthorRole: fieldName = Entry::ftAuthor; break;
        case YearRole: fieldName = Entry::ftYear; break;
        }
        if (!fieldName.isEmpty())
            return valueToText(curEntry->operator[](fieldName));

        if (role == BibTeXIdRole) {
            return curEntry->id();
        } else if (role == FoundViaRole) {
            const QString foundVia = valueToText(curEntry->operator[](QStringLiteral("x-fetchedfrom")));
            if (!foundVia.isEmpty()) return foundVia;
        } else if (role == AuthorRole) {
            const QString authors = valueToText(curEntry->operator[](Entry::ftAuthor));
            if (!authors.isEmpty()) return authors;
            else return valueToText(curEntry->operator[](Entry::ftEditor));
        } else if (role == WherePublishedRole) {
            const QString journal = valueToText(curEntry->operator[](Entry::ftJournal));
            if (!journal.isEmpty()) {
                const QString volume = valueToText(curEntry->operator[](Entry::ftVolume));
                const QString issue = valueToText(curEntry->operator[](Entry::ftNumber));
                if (volume.isEmpty())
                    return journal;
                else if (issue.isEmpty()) /// but 'volume' is not empty
                    return journal + QStringLiteral(" ") + volume;
                else /// both 'volume' and 'issue' are not empty
                    return journal + QStringLiteral(" ") + volume + QStringLiteral(" (") + issue + QStringLiteral(")");
            }
            const QString bookTitle = valueToText(curEntry->operator[](Entry::ftBookTitle));
            if (!bookTitle.isEmpty()) return bookTitle;
            const bool isPhdThesis = curEntry->type() == Entry::etPhDThesis;
            if (isPhdThesis) {
                const QString school = valueToText(curEntry->operator[](Entry::ftSchool));
                if (school.isEmpty())
                    return tr("Doctoral dissertation");
                else
                    return tr("Doctoral dissertation (%1)").arg(school);
            }
            const QString school = valueToText(curEntry->operator[](Entry::ftSchool));
            if (!school.isEmpty()) return school;
            const QString publisher = valueToText(curEntry->operator[](Entry::ftPublisher));
            if (!publisher.isEmpty()) return publisher;
            return QStringLiteral("");
        } else if (role == AuthorShortRole) {
            const QStringList authors = valueToList(curEntry->operator[](Entry::ftAuthor));
            switch (authors.size()) {
            case 0: return QString(); ///< empty list of authors
            case 1: return authors.first(); ///< single author
            case 2: return tr("%1 and %2").arg(authors.first()).arg(authors[1]); ///< two authors
            default: return tr("%1 and %2 more").arg(authors.first()).arg(QString::number(authors.size() - 1)); ///< three or more authors
            }
        } else if (role == UrlRole) {
            const QStringList doiList = valueToList(curEntry->operator[](Entry::ftDOI));
            if (!doiList.isEmpty()) return QStringLiteral("http://dx.doi.org/") + doiList.first();
            const QStringList urlList = valueToList(curEntry->operator[](Entry::ftUrl));
            if (!urlList.isEmpty()) return urlList.first();
            const QStringList bibUrlList = valueToList(curEntry->operator[](QStringLiteral("biburl")));
            if (!bibUrlList.isEmpty()) return bibUrlList.first();
            return QString();
        } else if (role == DoiRole) {
            const QStringList doiList = valueToList(curEntry->operator[](Entry::ftDOI));
            if (!doiList.isEmpty()) return doiList.first();
            return QString();
        }
    }

    return QVariant();
}

const QSharedPointer<const Entry> BibliographyModel::entry(int row) const {
    const QSharedPointer<const Element> element = m_file->at(row);
    const QSharedPointer<const Entry> result = element.dynamicCast<const Entry>();
    return result;
}

void BibliographyModel::startSearch(const QString &freeText, const QString &title, const QString &author) {
    QMap<QString, QString> query;
    query[OnlineSearchAbstract::queryKeyFreeText] = freeText;
    query[OnlineSearchAbstract::queryKeyTitle] = title;
    query[OnlineSearchAbstract::queryKeyAuthor] = author;

    m_runningSearches = 0;
    const QSettings settings(QStringLiteral("harbour-bibsearch"), QStringLiteral("BibSearch"));
    for (int i = 0; i < m_searchEngineList->size(); ++i) {
        OnlineSearchAbstract *osa = m_searchEngineList->at(i);
        const bool doSearchThisEngine = isSearchEngineEnabled(settings, osa);
        if (!doSearchThisEngine) continue;
        osa->startSearch(query, 10 /** TODO make number configurable */);
        ++m_runningSearches;
    }
}

void BibliographyModel::clear() {
    beginResetModel();
    m_file->clear();
    endResetModel();
}

bool BibliographyModel::isBusy() const {
    for (QVector<OnlineSearchAbstract *>::ConstIterator it = m_searchEngineList->constBegin(); it != m_searchEngineList->constEnd(); ++it) {
        if ((*it)->busy()) return true;
    }
    return false;
}

void BibliographyModel::searchFinished() {
    --m_runningSearches;
}

void BibliographyModel::newEntry(QSharedPointer<Entry> e) {
    const int n = m_file->count();
    beginInsertRows(QModelIndex(), n, n);
    m_file->insert(n, e);
    endInsertRows();
}

QString BibliographyModel::valueToText(const Value &value) {
    return valueToList(value).join(QStringLiteral(", "));
}


QStringList BibliographyModel::valueToList(const Value &value) {
    if (value.isEmpty()) return QStringList();

    QStringList resultItems;

    const QSharedPointer<const Person> firstPerson = value.first().dynamicCast<const Person>();
    if (!firstPerson.isNull()) {
        /// First item in value is a Person, assume all other items are Persons as well
        for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
            QSharedPointer<const Person> person = (*it).dynamicCast<const Person>();
            if (person.isNull()) continue;
            const QString name = personToText(person);
            if (name.isEmpty()) continue;
            resultItems.append(beautifyLaTeX(name));
        }
    } else {
        for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
            const QString valueItem = valueItemToText(*it);
            if (valueItem.isEmpty()) continue;
            resultItems.append(beautifyLaTeX(valueItem));
        }
    }

    return resultItems;
}

QString BibliographyModel::personToText(const QSharedPointer<const Person> &person) {
    if (person.isNull()) return QString();
    QString name = person->lastName();
    if (name.isEmpty()) return QString();
    const QString firstName = person->firstName().left(1);
    if (!firstName.isEmpty())
        name = name.prepend(QStringLiteral(". ")).prepend(firstName);
    return name;
}

QString BibliographyModel::valueItemToText(const QSharedPointer<ValueItem> &valueItem) {
    const QSharedPointer<PlainText> plainText = valueItem.dynamicCast<PlainText>();
    if (!plainText.isNull())
        return plainText->text();
    else {
        const QSharedPointer<VerbatimText> verbatimText = valueItem.dynamicCast<VerbatimText>();
        if (!verbatimText.isNull())
            return verbatimText->text();
        else {
            const QSharedPointer<MacroKey> macroKey = valueItem.dynamicCast<MacroKey>();
            if (!macroKey.isNull())
                return macroKey->text();
            else {
                // TODO
                return QString();
            }
        }
    }
}

QString BibliographyModel::beautifyLaTeX(const QString &input) {
    QString output = input;
    static const QStringList toBeRemoved {QStringLiteral("\\textsuperscript{"), QStringLiteral("\\}"), QStringLiteral("\\{"), QStringLiteral("}"), QStringLiteral("{")};
    for (QStringList::ConstIterator it = toBeRemoved.constBegin(); it != toBeRemoved.constEnd(); ++it)
        output = output.remove(*it);

    return output;
}
