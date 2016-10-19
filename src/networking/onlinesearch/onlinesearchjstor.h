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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/
#ifndef KBIBTEX_ONLINESEARCH_JSTOR_H
#define KBIBTEX_ONLINESEARCH_JSTOR_H

#include "onlinesearchabstract.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchJStor : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchJStor(QWidget *parent);
    ~OnlineSearchJStor();

    virtual void startSearch();
    virtual void startSearch(const QMap<QString, QString> &query, int numResults);
    virtual QString label() const;
    virtual OnlineSearchQueryFormAbstract *customWidget(QWidget *parent);
    virtual QUrl homepage() const;

public slots:
    void cancel();

protected:
    virtual QString favIconUrl() const;
    virtual void sanitizeEntry(QSharedPointer<Entry> entry);

private slots:
    void doneFetchingStartPage();
    void doneFetchingResultPage();
    void doneFetchingBibTeXCode();

private:
    class OnlineSearchJStorPrivate;
    OnlineSearchJStorPrivate *d;
};

#endif // KBIBTEX_ONLINESEARCH_JSTOR_H
