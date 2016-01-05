/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#include <QBuffer>
#include <QTextStream>
#include <QStringList>
#include <QRegExp>

#include <KDebug>

#include "value.h"
#include "fileimporter.h"

FileImporter::FileImporter()
        : QObject()
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
        kWarning() << "Cannot create File object from empty string";
        return NULL;
    }

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QTextStream stream(&buffer);
    stream.setCodec("UTF-8");
    stream << text;
    buffer.close();

    buffer.open(QIODevice::ReadOnly);
    File *result = load(&buffer);
    if (result == NULL)
        kWarning() << "Creating File object from" << buffer.size() << "Bytes of data failed";
    buffer.close();

    return result;
}

Person *FileImporter::splitName(const QString &name)
{
    // FIXME: This is a rather ugly code
    QStringList segments = name.split(QRegExp("[ ,]+"));
    bool containsComma = name.contains(',');
    QString firstName = "";
    QString lastName = "";

    if (segments.isEmpty())
        return NULL;

    if (!containsComma) {
        /** PubMed uses a special writing style for names, where the last name is followed by single capital letter,
          * each being the first letter of each first name
          * So, check how many single capital letters are at the end of the given segment list */
        int singleCapitalLettersCounter = 0;
        int p = segments.count() - 1;
        while (segments[p].length() == 1 && segments[p].compare(segments[p].toUpper()) == 0) {
            --p;
            ++singleCapitalLettersCounter;
        }

        if (singleCapitalLettersCounter > 0) {
            /** this is a special case for names from PubMed, which are formatted like "Fischer T"
              * all segment values until the first single letter segment are last name parts */
            for (int i = 0; i < p; ++i)
                lastName.append(segments[i]).append(" ");
            lastName.append(segments[p]);
            /** single letter segments are first name parts */
            for (int i = p + 1; i < segments.count() - 1; ++i)
                firstName.append(segments[i]).append(" ");
            firstName.append(segments[segments.count() - 1]);
        } else {
            int from = segments.count() - 1;
            lastName = segments[from];
            /** check for lower case parts of the last name such as "van", "von", "de", ... */
            while (from > 0) {
                if (segments[from - 1].compare(segments[from - 1].toLower()) != 0)
                    break;
                --from;
                lastName.prepend(" ");
                lastName.prepend(segments[from]);
            }

            if (from > 0) {
                /** there are segments left for the first name */
                firstName = *segments.begin();
                for (QStringList::Iterator it = ++segments.begin(); from > 1; ++it, --from) {
                    firstName.append(" ");
                    firstName.append(*it);
                }
            }
        }
    } else {
        bool inLastName = true;
        for (int i = 0; i < segments.count(); ++i) {
            if (segments[i] == ",")
                inLastName = false;
            else if (inLastName) {
                if (!lastName.isEmpty()) lastName.append(" ");
                lastName.append(segments[i]);
            } else {
                if (!firstName.isEmpty()) firstName.append(" ");
                firstName.append(segments[i]);
            }
        }
    }

    return new Person(firstName, lastName);
}

// #include "fileimporter.moc"
