/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef KBIBTEX_GUI_BIBTEXFIELDS_H
#define KBIBTEX_GUI_BIBTEXFIELDS_H

#include "kbibtexio_export.h"

#include <QString>
#include <QList>

#include "kbibtexnamespace.h"

struct FieldDescription {
    QString upperCamelCase;
    QString upperCamelCaseAlt;
    QString label;
    KBibTeX::TypeFlags typeFlags;
    KBibTeX::TypeFlag preferredTypeFlag;
    QMap<QString, int> width;
    int defaultWidth;
    QMap<QString, bool> visible;
    bool defaultVisible;
    bool typeIndependent;

    FieldDescription()
            : upperCamelCase(QString()), upperCamelCaseAlt(QString()), label(QString()), preferredTypeFlag(KBibTeX::tfSource), defaultWidth(0), defaultVisible(true), typeIndependent(false) {
        /* nothing */
    }

    bool isNull() const {
        return upperCamelCase.isEmpty() && label.isEmpty();
    }
};

bool operator==(const FieldDescription &a, const FieldDescription &b);
uint qHash(const FieldDescription &a);

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT BibTeXFields : public QList<FieldDescription *>
{
public:
    ~BibTeXFields();

    /**
     * Only one instance of this class has to be used
     * @return the class's singleton
     */
    static const BibTeXFields *self();

    void save();
    void resetToDefaults(const QString &treeViewName);

    /**
     * Change the casing of a given field name to one of the predefine formats.
     */
    QString format(const QString &name, KBibTeX::Casing casing) const;

    static KBibTeX::TypeFlag typeFlagFromString(const QString &typeFlagString);
    static KBibTeX::TypeFlags typeFlagsFromString(const QString &typeFlagsString);
    static QString typeFlagToString(KBibTeX::TypeFlag typeFlag);
    static QString typeFlagsToString(KBibTeX::TypeFlags typeFlags);

    const FieldDescription *find(const QString &name) const;

protected:
    BibTeXFields();

private:
    class BibTeXFieldsPrivate;
    BibTeXFieldsPrivate *d;
};

#endif // KBIBTEX_GUI_BIBTEXFIELDS_H