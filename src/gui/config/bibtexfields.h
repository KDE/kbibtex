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
#ifndef KBIBTEX_GUI_BIBTEXFIELDS_H
#define KBIBTEX_GUI_BIBTEXFIELDS_H

#include <QString>
#include <QList>

namespace KBibTeX
{
namespace GUI {
namespace Config {

typedef struct {
    QString raw;
    QString rawAlt;
    QString label;
    int width;
    int defaultWidth;
    bool visible;
} FieldDescription;

/**
@author Thomas Fischer
*/
class BibTeXFields : public QList<FieldDescription>
{
public:
    enum Casing {cSmall, cCaptial, cCamelCase};

    virtual ~BibTeXFields();

    static BibTeXFields *self();
    void load();
    void save();
    void resetToDefaults();

    /**
     * Change the casing of a given field name to one of the predefine formats.
     */
    QString format(const QString& name, Casing casing) const;

protected:
    BibTeXFields();

private:
    class BibTeXFieldsPrivate;
    BibTeXFieldsPrivate *d;
};
}
}
}

#endif // KBIBTEX_GUI_BIBTEXFIELDS_H
