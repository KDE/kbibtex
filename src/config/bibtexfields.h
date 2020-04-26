/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_CONFIG_BIBTEXFIELDS_H
#define KBIBTEX_CONFIG_BIBTEXFIELDS_H

#include <QString>
#include <QVector>

#include <KBibTeX>

#ifdef HAVE_KF5
#include "kbibtexconfig_export.h"
#endif // HAVE_KF5

typedef struct {
    /**
     * Name of this field in 'upper camel case', e.g. 'BookTitle', but not
     * 'booktitle', 'BOOKTITLE', or 'BoOkTiTlE'.
     */
    QString upperCamelCase;
    /**
     * The 'alternative' field name is used to create 'meta fields', such as
     * 'author' or 'editor' combined. It shall not be used to define aliases
     * such as 'pdf' to be an alias for 'file'.
     */
    QString upperCamelCaseAlt;
    /**
     * List of aliases for a field. Empty for most fields. Alias for BibLaTeX
     * must be explictly mentioned in BibLaTeX's official documentation, see
     * section 'Field Aliases'.
     * Field aliases shall not be used as alternative field names.
     */
    QStringList upperCamelCaseAliases;
    /**
     * Localized (translated) name of this field.
     */
    QString label;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;
    QMap<QString, int> visualIndex;
    QMap<QString, int> width;
    int defaultWidth;
    QMap<QString, bool> visible;
    bool defaultVisible;
    bool typeIndependent;
} FieldDescription;

bool operator==(const FieldDescription &a, const FieldDescription &b);
uint qHash(const FieldDescription &a);

/**
@author Thomas Fischer
 */
class KBIBTEXCONFIG_EXPORT BibTeXFields : public QVector<FieldDescription>
{
public:
    ~BibTeXFields();

    /**
     * Only one instance of this class has to be used
     * @return the class's singleton
     */
    static BibTeXFields &instance();

#ifdef HAVE_KF5
    void save();
    void resetToDefaults(const QString &treeViewName);
#endif // HAVE_KF5

    /**
     * Change the casing of a given field name to one of the predefine formats.
     */
    QString format(const QString &name, KBibTeX::Casing casing) const;

    static KBibTeX::TypeFlag typeFlagFromString(const QString &typeFlagString);
    static KBibTeX::TypeFlags typeFlagsFromString(const QString &typeFlagsString);
    static QString typeFlagToString(KBibTeX::TypeFlag typeFlag);
    static QString typeFlagsToString(KBibTeX::TypeFlags typeFlags);

    const FieldDescription find(const QString &name) const;

private:
    Q_DISABLE_COPY(BibTeXFields)

    explicit BibTeXFields(const QString &style, const QVector<FieldDescription> &other);

    class BibTeXFieldsPrivate;
    BibTeXFieldsPrivate *d;
};

#endif // KBIBTEX_CONFIG_BIBTEXFIELDS_H
