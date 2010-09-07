/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#ifndef KBIBTEX_WEBSEARCH_ABSTRACT_H
#define KBIBTEX_WEBSEARCH_ABSTRACT_H

#include "kbibtexio_export.h"

#include <QObject>
#include <QMap>
#include <QString>

#include <entry.h>

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT WebSearchAbstract : public QObject
{
    Q_OBJECT

public:
    static const QString queryKeyFreeText;
    static const QString queryKeyTitle;
    static const QString queryKeyAuthor;
    static const QString queryKeyYear;

    static const int resultCancelled;
    static const int resultNoError;
    static const int resultUnspecifiedError;

    virtual void startSearch(const QMap<QString, QString> &query, int numResults) = 0;
    virtual QString label() const = 0;

signals:
    void foundEntry(Entry*);
    void stoppedSearch(int);
};

#endif // KBIBTEX_WEBSEARCH_ABSTRACT_H
