/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHINGENTACONNECT_H
#define KBIBTEX_NETWORKING_ONLINESEARCHINGENTACONNECT_H

#include <QByteArray>

#include <onlinesearch/OnlineSearchAbstract>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchIngentaConnect : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchIngentaConnect(QObject *parent);
    ~OnlineSearchIngentaConnect() override;

#ifdef HAVE_QTWIDGETS
    void startSearchFromForm() override;
#endif // HAVE_QTWIDGETS
    void startSearch(const QMap<QString, QString> &query, int numResults) override;
    QString label() const override;
#ifdef HAVE_QTWIDGETS
    OnlineSearchAbstract::Form *customWidget(QWidget *parent) override;
#endif // HAVE_QTWIDGETS
    QUrl homepage() const override;

protected:
    QString favIconUrl() const override;

private:
#ifdef HAVE_QTWIDGETS
    class Form;
#endif // HAVE_QTWIDGETS
    class OnlineSearchIngentaConnectPrivate;
    OnlineSearchIngentaConnectPrivate *d;

private slots:
    void downloadDone();
};

#endif // KBIBTEX_NETWORKING_ONLINESEARCHINGENTACONNECT_H
