/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "journalabbreviations.h"

#include <QHash>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QStandardPaths>

#include "logging_processing.h"

class JournalAbbreviations::Private
{
private:
    const QString journalFilename;

    QHash<QString, QString> leftToRightMap, rightToLeftMap;

public:
    Private(JournalAbbreviations *parent)
            : journalFilename(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kbibtex/jabref_journalabbrevlist.txt")))
    {
        Q_UNUSED(parent)
    }

    bool loadMapping() {
        leftToRightMap.clear();
        rightToLeftMap.clear();

        QFile journalFile(journalFilename);
        if (journalFile.open(QFile::ReadOnly)) {
            static const QRegExp splitRegExp(QStringLiteral("\\s*[=;]\\s*"));

            QTextStream ts(&journalFile);
            ts.setCodec("utf8");

            QString line;
            while (!(line = ts.readLine()).isNull()) {
                /// Skip empty lines or comments
                if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
                QStringList columns = line.split(splitRegExp);
                /// Skip lines that do not have at least two columns
                if (columns.count() < 2) continue;

                const QString alreadyInLeftToRightMap = leftToRightMap[columns[0]];
                if (!alreadyInLeftToRightMap.isEmpty()) {
                    if (alreadyInLeftToRightMap.length() > columns[1].length()) {
                        leftToRightMap.remove(columns[0]);
                        leftToRightMap.insert(columns[0], columns[1]);
                    }
                } else
                    leftToRightMap.insert(columns[0], columns[1]);
                rightToLeftMap.insert(columns[1], columns[0]);
            }

            journalFile.close();

            return !leftToRightMap.isEmpty();
        } else {
            qCWarning(LOG_KBIBTEX_PROCESSING) << "Cannot open journal abbreviation list file at" << journalFilename;
            return false;
        }
    }

    QString leftToRight(const QString &left) {
        if (leftToRightMap.isEmpty())
            loadMapping();
        return leftToRightMap.value(left, left);
    }

    QString rightToLeft(const QString &right) {
        if (rightToLeftMap.isEmpty())
            loadMapping();
        return rightToLeftMap.value(right, right);
    }
};

JournalAbbreviations *JournalAbbreviations::instance = nullptr;

JournalAbbreviations::JournalAbbreviations()
        : d(new JournalAbbreviations::Private(this))
{
    /// nothing
}

JournalAbbreviations::~JournalAbbreviations()
{
    delete d;
}

JournalAbbreviations *JournalAbbreviations::self() {
    if (instance == nullptr)
        instance = new JournalAbbreviations();
    return instance;
}

QString JournalAbbreviations::toShortName(const QString &longName) const {
    return d->leftToRight(longName);
}

QString JournalAbbreviations::toLongName(const QString &shortName) const {
    return d->rightToLeft(shortName);
}
