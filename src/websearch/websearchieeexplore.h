/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#ifndef KBIBTEX_WEBSEARCH_IEEEXPLORE_H
#define KBIBTEX_WEBSEARCH_IEEEXPLORE_H

#include "websearchabstract.h"

class KJob;
namespace KIO
{
class Job;
}

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT WebSearchIEEEXplore : public WebSearchAbstract
{
    Q_OBJECT

public:
    WebSearchIEEEXplore(QWidget *parent);

    virtual void startSearch();
    virtual void startSearch(const QMap<QString, QString> &query, int numResults);
    virtual QString label() const;
    virtual WebSearchQueryFormAbstract* customWidget(QWidget *parent);
    virtual KUrl homepage() const;

public slots:
    void cancel();

protected:
    virtual QString favIconUrl() const;

private slots:
    void doneFetchingStartPage(KJob *);
    void doneFetchingSearchResults(KJob *);
    void doneFetchingAbstract(KJob *);
    void doneFetchingBibliography(KJob *);
    void permanentRedirection(KIO::Job *, const KUrl &, const KUrl &);
    void redirection(KIO::Job *, const KUrl &);

private:
    class WebSearchQueryFormIEEEXplore;
    WebSearchQueryFormIEEEXplore *form;
    class WebSearchIEEEXplorePrivate;
    WebSearchIEEEXplorePrivate *d;
};

#endif // KBIBTEX_WEBSEARCH_IEEEXPLORE_H
