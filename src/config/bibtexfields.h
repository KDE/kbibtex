/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "kbibtexconfig_export.h"

#include <QString>
#include <QVector>

#include "kbibtex.h"

typedef struct {
    QString upperCamelCase;
    QString upperCamelCaseAlt;
    QString label;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;
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
    BibTeXFields(const BibTeXFields &other) = delete;
    BibTeXFields &operator= (const BibTeXFields &other) = delete;
    ~BibTeXFields();

    /**
     * Only one instance of this class has to be used
     * @return the class's singleton
     */
    static BibTeXFields *self();

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
    explicit BibTeXFields(const QVector<FieldDescription> &other);

    class BibTeXFieldsPrivate;
    BibTeXFieldsPrivate *d;
};

#endif // KBIBTEX_CONFIG_BIBTEXFIELDS_H
