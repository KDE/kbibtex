/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_ONLINESEARCH_BIBSONOMY_H
#define KBIBTEX_ONLINESEARCH_BIBSONOMY_H

#include "onlinesearchabstract.h"

#ifdef HAVE_QTWIDGETS
class QSpinBox;

#ifdef HAVE_KF5
class KComboBox;
class KLineEdit;
#endif // HAVE_KF5
#endif // HAVE_QTWIDGETS

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchBibsonomy : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchBibsonomy(QObject *parent);
    ~OnlineSearchBibsonomy() override;

#ifdef HAVE_QTWIDGETS
    void startSearchFromForm() override;
#endif // HAVE_QTWIDGETS
    void startSearch(const QMap<QString, QString> &query, int numResults) override;
    QString label() const override;
#ifdef HAVE_QTWIDGETS
    OnlineSearchQueryFormAbstract *customWidget(QWidget *parent) override;
#endif // HAVE_QTWIDGETS
    QUrl homepage() const override;

protected:
    QString favIconUrl() const override;

private:
#ifdef HAVE_QTWIDGETS
    class OnlineSearchQueryFormBibsonomy;
#endif // HAVE_QTWIDGETS
    class OnlineSearchBibsonomyPrivate;
    OnlineSearchBibsonomyPrivate *d;

private slots:
    void downloadDone();
};

#endif // KBIBTEX_ONLINESEARCH_BIBSONOMY_H
