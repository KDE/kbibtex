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

#ifndef KBIBTEX_PART_PARTFACTORY_H
#define KBIBTEX_PART_PARTFACTORY_H

#include <kparts/factory.h>

class KComponentData;

class KBibTeXPartFactory : public KParts::Factory
{
    Q_OBJECT

public:
    KBibTeXPartFactory();
    virtual ~KBibTeXPartFactory();

public:
    virtual KParts::Part *createPartObject(QWidget *parentWidget, QObject *parent, const char *classname, const QStringList &args);

public:
    static const KComponentData &componentData();
};

#endif // KBIBTEX_PART_PARTFACTORY_H
