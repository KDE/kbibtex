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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHOCLCWORLDCAT_H
#define KBIBTEX_NETWORKING_ONLINESEARCHOCLCWORLDCAT_H

#include "onlinesearchabstract.h"

/**
 * According to its own description at oclc.org, "OCLC is a worldwide library
 * cooperative, providing services and research to improve access to the
 * world's information."
 * Access to their services is available through the developers' portal at
 * https://oclc.org/developer/
 * WorldCat, according to OCLC's description, "connects library users to
 * hundreds of millions of electronic resources, including e-books, licensed
 * databases, online periodicals and collections of digital items."
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchOCLCWorldCat : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchOCLCWorldCat(QWidget *parent);
    ~OnlineSearchOCLCWorldCat();

    virtual void startSearch();
    virtual void startSearch(const QMap<QString, QString> &query, int numResults);
    virtual QString label() const;
    virtual OnlineSearchQueryFormAbstract *customWidget(QWidget *parent);
    virtual QUrl homepage() const;

public slots:
    void cancel();

protected:
    virtual QString favIconUrl() const;

private slots:
    void downloadDone();

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_NETWORKING_ONLINESEARCHOCLCWORLDCAT_H
