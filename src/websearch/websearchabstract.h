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
#include <QWidget>
#include <QMetaType>

#include <KIcon>

#include <entry.h>

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class WebSearchQueryFormAbstract : public QWidget
{
public:
    WebSearchQueryFormAbstract(QWidget *parent)
            : QWidget(parent) {
        // nothing
    }

    virtual bool readyToStart() const {
        return false;
    }
};

Q_DECLARE_METATYPE(WebSearchQueryFormAbstract*)

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT WebSearchAbstract : public QObject
{
    Q_OBJECT

public:
    WebSearchAbstract(QWidget *parent) {
        m_parent = parent;
    }

    static const QString queryKeyFreeText;
    static const QString queryKeyTitle;
    static const QString queryKeyAuthor;
    static const QString queryKeyYear;

    static const int resultCancelled;
    static const int resultNoError;
    static const int resultUnspecifiedError;

    virtual void startSearch() = 0;
    virtual void startSearch(const QMap<QString, QString> &query, int numResults) = 0;
    virtual QString label() const = 0;
    virtual KIcon icon() const;
    virtual WebSearchQueryFormAbstract* customWidget(QWidget *parent) = 0;
    virtual KUrl homepage() const = 0;

protected:
    QWidget *m_parent;

    virtual QString favIconUrl() const = 0;

signals:
    void foundEntry(Entry*);
    void stoppedSearch(int);
};

#endif // KBIBTEX_WEBSEARCH_ABSTRACT_H
