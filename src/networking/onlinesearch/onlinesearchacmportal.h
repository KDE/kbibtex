/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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
#ifndef KBIBTEX_ONLINESEARCH_ACMPORTAL_H
#define KBIBTEX_ONLINESEARCH_ACMPORTAL_H

#include <QByteArray>

#include "onlinesearchabstract.h"

class QSpinBox;
class KComboBox;
class KLineEdit;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchAcmPortal : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    OnlineSearchAcmPortal(QWidget *parent);
    ~OnlineSearchAcmPortal();

    virtual void startSearch();
    virtual void startSearch(const QMap<QString, QString> &query, int numResults);
    virtual QString label() const;
    virtual OnlineSearchQueryFormAbstract *customWidget(QWidget *parent);
    virtual KUrl homepage() const;

public slots:
    void cancel();

protected:
    virtual QString favIconUrl() const;

private slots:
    void doneFetchingStartPage();
    void doneFetchingSearchPage();
    void doneFetchingBibTeX();

private:
    class OnlineSearchAcmPortalPrivate;
    OnlineSearchAcmPortalPrivate *d;
};

#endif // KBIBTEX_ONLINESEARCH_ACMPORTAL_H
