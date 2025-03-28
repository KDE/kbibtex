/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHSPRINGERLINK_H
#define KBIBTEX_NETWORKING_ONLINESEARCHSPRINGERLINK_H

#include <onlinesearch/OnlineSearchAbstract>

#ifdef HAVE_KF
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 *
 * See also: https://dev.springernature.com/
 *
 * On the subject of having multiple "constraints" (search terms) in
 * a search, Springer's documentation states: "Each constraint that
 * appears in your request will automatically be AND'ed with all the others
 * For instance, a request including constraints: "title:bone+name:Jones"
 * is the equivalent to the request containing constraints concatenated by
 * the AND operator: "title:bone%20AND%20name:Jones".
 * (source: https://dev.springernature.com/adding-constraints)
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchSpringerLink : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchSpringerLink(QObject *parent);
    ~OnlineSearchSpringerLink() override;

#ifdef HAVE_QTWIDGETS
    void startSearchFromForm() override;
#endif // HAVE_QTWIDGETS
    void startSearch(const QMap<QueryKey, QString> &query, int numResults) override;
    QString label() const override;
#ifdef HAVE_QTWIDGETS
    OnlineSearchAbstract::Form *customWidget(QWidget *parent) override;
#endif // HAVE_QTWIDGETS
    QUrl homepage() const override;

#ifdef BUILD_TESTING
    // KBibTeXNetworkingTest::onlineSearchSpringerLinkXMLparsing  makes use of these functions to test parsing XML data
    QVector<QSharedPointer<Entry>> parsePAM(const QByteArray &xmlData, bool *ok = nullptr);
    static QByteArray rewriteXMLformatting(const QByteArray &input);
#endif // BUILD_TESTING

private Q_SLOTS:
    void doneFetchingPAM();

private:
#ifdef HAVE_QTWIDGETS
    class Form;
#endif // HAVE_QTWIDGETS

    class OnlineSearchSpringerLinkPrivate;
    OnlineSearchSpringerLinkPrivate *d;
};

#endif // KBIBTEX_NETWORKING_ONLINESEARCHSPRINGERLINK_H
