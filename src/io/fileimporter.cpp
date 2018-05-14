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

#include "fileimporter.h"

#include <QBuffer>
#include <QTextStream>
#include <QStringList>
#include <QRegExp>

#include "value.h"
#include "logging_io.h"

FileImporter::FileImporter(QObject *parent)
        : QObject(parent)
{
    // nothing
}

FileImporter::~FileImporter()
{
    // nothing
}

File *FileImporter::fromString(const QString &text)
{
    if (text.isEmpty()) {
        qCWarning(LOG_KBIBTEX_IO) << "Cannot create File object from empty string";
        return nullptr;
    }

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QTextStream stream(&buffer);
    stream.setCodec("UTF-8");
    stream << text;
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    File *result = load(&buffer);
    if (result == nullptr)
        qCWarning(LOG_KBIBTEX_IO) << "Creating File object from" << buffer.size() << "Bytes of data failed";
    buffer.close();

    return result;
}

Person *FileImporter::splitName(const QString &name)
{
    QString firstName;
    QString lastName;
    QString suffix;

    if (!name.contains(QLatin1Char(','))) {
        const QStringList segments = name.split(QRegExp("[ ]+"));

        /** PubMed uses a special writing style for names, where the last name is followed by
          * single capital letters, each being the first letter of each first name
          * So, check how many single capital letters are at the end of the given segment list */
        int singleCapitalLettersCounter = 0;
        int p = segments.count() - 1;
        while (segments[p].length() == 1 && segments[p][0].isUpper()) {
            --p;
            ++singleCapitalLettersCounter;
        }

        if (singleCapitalLettersCounter > 0) {
            /** This is a special case for names from PubMed, which are formatted like "Fischer T A"
              * all segment values until the first single letter segment are last name parts */
            for (int i = 0; i < p; ++i)
                lastName.append(segments[i]).append(QStringLiteral(" "));
            lastName.append(segments[p]);
            /// Single letter segments are first name parts
            for (int i = p + 1; i < segments.count() - 1; ++i)
                firstName.append(segments[i]).append(QStringLiteral(" "));
            firstName.append(segments[segments.count() - 1]);
        } else {
            int from = segments.count() - 1;
            if (looksLikeSuffix(segments[from])) {
                suffix = segments[from];
                --from;
            }
            lastName = segments[from]; ///< Initialize last name with last segment
            /// Check for lower case parts of the last name such as "van", "von", "de", ...
            while (from > 0) {
                if (segments[from - 1].compare(segments[from - 1].toLower()) != 0)
                    break;
                --from;
                lastName.prepend(QStringLiteral(" "));
                lastName.prepend(segments[from]);
            }

            if (from > 0) {
                firstName = *segments.begin(); /// First name initialized with first segment
                for (QStringList::ConstIterator it = ++segments.begin(); from > 1; ++it, --from) {
                    firstName.append(" ");
                    firstName.append(*it);
                }
            }
        }
    } else {
        const QStringList segments = name.split(QStringLiteral(","));
        /// segments.count() must be >=2
        if (segments.count() == 2) {
            /// Most probably "Smith, Adam"
            lastName = segments[0].trimmed();
            firstName = segments[1].trimmed();
        } else if (segments.count() == 3 && looksLikeSuffix(segments[2])) {
            /// Most probably "Smith, Adam, Jr."
            lastName = segments[0].trimmed();
            firstName = segments[1].trimmed();
            suffix = segments[2].trimmed();
        } else
            qWarning() << "Too many commas in name:" << name;
    }

    return new Person(firstName, lastName, suffix);
}

bool FileImporter::looksLikeSuffix(const QString &suffix)
{
    const QString normalizedSuffix = suffix.trimmed().toLower();
    return normalizedSuffix == QStringLiteral("jr")
           || normalizedSuffix == QStringLiteral("jr.")
           || normalizedSuffix == QStringLiteral("sr")
           || normalizedSuffix == QStringLiteral("sr.")
           || normalizedSuffix == QStringLiteral("ii")
           || normalizedSuffix == QStringLiteral("iii")
           || normalizedSuffix == QStringLiteral("iv");
}

// #include "fileimporter.moc"
