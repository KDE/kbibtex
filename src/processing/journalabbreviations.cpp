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
#include <QRegularExpression>
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
            static const QRegularExpression splitRegExp(QStringLiteral("\\s*[=;]\\s*"));

            QTextStream ts(&journalFile);
            ts.setCodec("utf8");

            QString line;
            while (!(line = ts.readLine().trimmed()).isNull()) {
                /// Skip empty lines or comments
                if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
                const QStringList columns = line.split(splitRegExp);
                /// Skip lines that do not have at least two columns
                if (columns.count() < 2) continue;
                /// At this point, a given line like
                ///    Accounts of Chemical Research=Acc. Chem. Res.;ACHRE4;M
                /// may have been split into the columns of
                ///    Accounts of Chemical Research
                ///    Acc. Chem. Res.
                ///    ACHRE4
                ///    M
                /// The last two colums are optional and are not processed here.
                /// The first column is the journal's full name, the second column
                /// is its abbreviation.
                /// QHash leftToRightMap maps from full name to abbreviation,
                /// QHash rightToLeftMap maps from abbreviation to full name.

                const QString alreadyInLeftToRightMap = leftToRightMap[columns[0]];
                if (!alreadyInLeftToRightMap.isEmpty()) {
                    if (alreadyInLeftToRightMap.length() > columns[1].length())
                        /// If there is already an existing mapping from full name to
                        /// abbreviation, replace it if the newly found abbreviation
                        /// is longer.
                        leftToRightMap.insert(columns[0], columns[1]);
                } else
                    /// Previously unknown journal full name, so add it to mapping.
                    leftToRightMap.insert(columns[0], columns[1]);
                /// Always add/replace mapping from abbreviation to full name.
                rightToLeftMap.insert(columns[1], columns[0]);
            }

            journalFile.close();

            /// Success means at least one mapping has been recorded.
            return !leftToRightMap.isEmpty();
        } else {
            qCWarning(LOG_KBIBTEX_PROCESSING) << "Cannot open journal abbreviation list file at" << journalFilename;
            return false;
        }
    }

    QString leftToRight(const QString &left) {
        if (leftToRightMap.isEmpty())
            loadMapping(); ///< lazy loading of mapping, i.e. only when data gets requested the first time
        return leftToRightMap.value(left, left);
    }

    QString rightToLeft(const QString &right) {
        if (rightToLeftMap.isEmpty())
            loadMapping(); ///< lazy loading of mapping, i.e. only when data gets requested the first time
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
