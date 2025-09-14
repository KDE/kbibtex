/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2024 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "isbn.h"

#include <QRegularExpression>
#include <QDebug>

#include "logging_networking.h"

QString ISBN::locate(const QString &haystack)
{
    if (haystack.isEmpty())
        // Easy case: haystack is empty
        return QString();

    static const QRegularExpression isbnRegExp(QStringLiteral("\\b(97[98][ -]?)?(\\d{1,6}[ -]?){3}[0-9Xx]\\b"));

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    auto it = isbnRegExp.globalMatch(haystack);
    while (it.hasNext()) {
        const QRegularExpressionMatch &match = it.next();
#else // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    for (const QRegularExpressionMatch &match : isbnRegExp.globalMatch(haystack)) {
#endif // QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const QString needle {match.captured().remove(u'-').remove(u' ')};
        // ISBNs may be either 10 or 13 characters long, with different checksum algorithms each
        if (needle.length() == 10) {
            // For potential ISBN-10 needles, perform checksum algorithm
            int s = 0;
            for (int i = 0, t = 0; i < 10; ++i) {
                t += needle[i].toUpper() == u'X' ? 10 : needle[i].digitValue();
                s += t;
            }
            if (s % 11 == 0)
                return needle;
            else
                qCInfo(LOG_KBIBTEX_NETWORKING) << "ISBN-10 needle did not pass checksum validation:" << needle;
        } else if (needle.length() == 13) {
            // For potential ISBN-13 needles, perform checksum algorithm
            int s = 0;
            for (int i = 0; i < 12; ++i) {
                s += needle[i].digitValue() * (i % 2 == 0 ? 1 : 3);
            }
            if ((10 - (s % 10)) % 10 == needle[12].digitValue())
                return needle;
            else
                qCWarning(LOG_KBIBTEX_NETWORKING) << "ISBN-13 needle did not pass checksum validation:" << needle;
        }
    }

    return QString();
}
