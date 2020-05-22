/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_CONFIG_BIBTEXENTRIES_H
#define KBIBTEX_CONFIG_BIBTEXENTRIES_H

#include <QStringList>
#include <QVector>

#include <KBibTeX>

#include "preferences.h"

#ifdef HAVE_KF5
#include "kbibtexconfig_export.h"
#endif // HAVE_KF5

typedef struct {
    QString upperCamelCase;
    QString upperCamelCaseAlt;
    QString label;
    QStringList requiredItems;
    QStringList optionalItems;
} EntryDescription;

bool operator==(const EntryDescription &a, const EntryDescription &b);
uint qHash(const EntryDescription &a);

/**
@author Thomas Fischer
 */
class KBIBTEXCONFIG_EXPORT BibTeXEntries : public QVector<EntryDescription>
{
public:
    virtual ~BibTeXEntries();

    /**
     * Only one instance of this class has to be used
     * @return the class's singleton
     */
    static const BibTeXEntries &instance();

    /**
     * Change the casing of a given entry name to one of the predefine formats.
     *
     */
    /**
     * Change the casing of a given entry name to one of the predefine formats.
     * @param name entry name to format
     * @param casing can be any of the predefined formats such as lower camel case or upper case
     * @return returns the formatted entry name if possible or the "name" parameter's value as fall-back
     */
    QString format(const QString &name, KBibTeX::Casing casing = Preferences::instance().bibTeXKeywordCasing()) const;

    /**
     * Returns the given entry name's i18n'ized, human-readable label,
     * for example "Journal Article" for entry name "article".
     * @param name entry name to look up the label for
     * @return the label for the entry if available, else an empty string
     */
    QString label(const QString &name) const;

private:
    Q_DISABLE_COPY(BibTeXEntries)

    explicit BibTeXEntries(const QVector<EntryDescription> &other);

    class BibTeXEntriesPrivate;
    BibTeXEntriesPrivate *d;
};

#endif // KBIBTEX_CONFIG_BIBTEXENTRIES_H
