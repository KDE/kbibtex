/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                 2014 Pavel Zorin-Kranich <pzorin@math.uni-bonn.de>      *
 *                 2018 Alexander Dunlap <alexander.dunlap@gmail.com>      *
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

#ifndef KBIBTEX_ONLINESEARCH_MRLOOKUP_H
#define KBIBTEX_ONLINESEARCH_MRLOOKUP_H

#include "onlinesearchabstract.h"

/**
 * @author Pavel Zorin-Kranich <pzorin@math.uni-bonn.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchMRLookup : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchMRLookup(QWidget *parent);

    void startSearch(const QMap<QString, QString> &query, int numResults) override;
    QString label() const override;
    QUrl homepage() const override;

protected:
    QString favIconUrl() const override;
    void sanitizeEntry(QSharedPointer<Entry> entry) override;

private slots:
    void doneFetchingResultPage();

private:
    static const QString queryUrlStem;
};

#endif // KBIBTEX_ONLINESEARCH_MRLOOKUP_H
