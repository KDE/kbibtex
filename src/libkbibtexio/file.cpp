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

#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QStringList>
#include <QRegExp>

#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>

#include <file.h>
#include <entry.h>
#include <element.h>
#include <macro.h>
#include <fileexporterbibtex.h>
#include <comment.h>

const QString File::Url = QLatin1String("Url");
const QString File::Encoding = QLatin1String("Encoding");
const QString File::StringDelimiter = QLatin1String("StringDelimiter");
const QString File::QuoteComment = QLatin1String("QuoteComment");
const QString File::KeywordCasing = QLatin1String("KeywordCasing");
const QString File::ProtectCasing = QLatin1String("ProtectCasing");
const QString File::NameFormatting = QLatin1String("NameFormatting");

class File::FilePrivate
{
private:
    File *p;

    KSharedConfigPtr config;
    const QString configGroupName;

    void loadConfiguration() {
        /// Load and set configuration as stored in settings
        KConfigGroup configGroup(config, configGroupName);
        properties.insert(File::Encoding, configGroup.readEntry(FileExporterBibTeX::keyEncoding, FileExporterBibTeX::defaultEncoding));
        properties.insert(File::StringDelimiter, configGroup.readEntry(FileExporterBibTeX::keyStringDelimiter, FileExporterBibTeX::defaultStringDelimiter));
        properties.insert(File::QuoteComment, (FileExporterBibTeX::QuoteComment)configGroup.readEntry(FileExporterBibTeX::keyQuoteComment, (int)FileExporterBibTeX::defaultQuoteComment));
        properties.insert(File::KeywordCasing, (KBibTeX::Casing)configGroup.readEntry(FileExporterBibTeX::keyKeywordCasing, (int)FileExporterBibTeX::defaultKeywordCasing));
        properties.insert(File::NameFormatting,  configGroup.readEntry(Person::keyPersonNameFormatting, ""));
        properties.insert(File::ProtectCasing, configGroup.readEntry(FileExporterBibTeX::keyProtectCasing, FileExporterBibTeX::defaultProtectCasing));
    }

public:
    QMap<QString, QVariant> properties;

    FilePrivate(File *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("FileExporterBibTeX")) {
        loadConfiguration();
    }

};

File::File()
        : QList<Element*>(), d(new FilePrivate(this))
{
    // nothing
}

File::File(const File &other)
        : QList<Element*>(other), d(new FilePrivate(this))
{
    // nothing
}

File::~File()
{
    // FIXME: at some point elements have to be deleted
    // maybe use QSharedPointer?
    //while (!isEmpty()) {
    //    Element *e = first();
    //    removeFirst();
    //    delete e;
    //}
}

const Element *File::containsKey(const QString &key, ElementTypes elementTypes) const
{
    for (ConstIterator it = begin(); it != end(); ++it) {
        const Entry* entry = elementTypes.testFlag(etEntry) ? dynamic_cast<const Entry*>(*it) : NULL;
        if (entry != NULL) {
            if (entry->id() == key)
                return entry;
        } else {
            const Macro* macro = elementTypes.testFlag(etMacro) ? dynamic_cast<const Macro*>(*it) : NULL;
            if (macro != NULL) {
                if (macro->key() == key)
                    return macro;
            }
        }
    }

    return NULL;
}

QStringList File::allKeys(ElementTypes elementTypes) const
{
    QStringList result;

    foreach(const Element *element, *this) {
        const Entry* entry = elementTypes.testFlag(etEntry) ? dynamic_cast<const Entry*>(element) : NULL;
        if (entry != NULL)
            result.append(entry->id());
        else {
            const Macro* macro = elementTypes.testFlag(etMacro) ? dynamic_cast<const Macro*>(element) : NULL;
            if (macro != NULL)
                result.append(macro->key());
        }
    }

    return result;
}

QSet<QString> File::uniqueEntryValuesSet(const QString &fieldName) const
{
    QSet<QString> valueSet;
    const QString lcFieldName = fieldName.toLower();

    foreach(const Element *element, *this) {
        const Entry* entry = dynamic_cast<const Entry*>(element);
        if (entry != NULL)
            for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it)
                if (it.key().toLower() == lcFieldName)
                    foreach(const QSharedPointer<ValueItem> &valueItem, it.value()) {
                    valueSet.insert(PlainTextValue::text(*valueItem, this));
                }
    }

    return valueSet;
}

QStringList File::uniqueEntryValuesList(const QString &fieldName) const
{
    QSet<QString> valueSet = uniqueEntryValuesSet(fieldName);
    QStringList list = valueSet.toList();
    list.sort();
    return list;
}

void File::setProperty(const QString &key, const QVariant &value)
{
    d->properties.insert(key, value);
}

QVariant File::property(const QString &key) const
{
    return d->properties.value(key);
}

QVariant File::property(const QString &key, const QVariant &defaultValue) const
{
    return d->properties.value(key, defaultValue);
}

bool File::hasProperty(const QString &key) const
{
    return d->properties.contains(key);
}
