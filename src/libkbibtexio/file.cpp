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

#include <file.h>
#include <entry.h>
#include <element.h>
#include <macro.h>
#include <comment.h>

const QString File::Url = QLatin1String("Url");
const QString File::Encoding = QLatin1String("Encoding");

class File::FilePrivate
{
private:
    File *p;

public:
    QMap<QString, QVariant> properties;

    FilePrivate(File *parent)
            : p(parent)        {
        // TODO
    }
};

File::File()
        : QList<Element*>(), d(new FilePrivate(this))
{
    // nothing
}

File::~File()
{
    // nothing
}

const Element *File::containsKey(const QString &key) const
{
    for (ConstIterator it = begin(); it != end(); ++it) {
        const Entry* entry = dynamic_cast<const Entry*>(*it);
        if (entry != NULL) {
            if (entry->id() == key)
                return entry;
        } else {
            const Macro* macro = dynamic_cast<const Macro*>(*it);
            if (macro != NULL) {
                if (macro->key() == key)
                    return macro;
            }
        }
    }

    return NULL;
}

QStringList File::allKeys() const
{
    QStringList result;

    for (ConstIterator it = begin(); it != end(); ++it) {
        const Entry* entry = dynamic_cast<const Entry*>(*it);
        if (entry != NULL)
            result.append(entry->id());
        else {
            const Macro* macro = dynamic_cast<const Macro*>(*it);
            if (macro != NULL)
                result.append(macro->key());
        }
    }

    return result;
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
