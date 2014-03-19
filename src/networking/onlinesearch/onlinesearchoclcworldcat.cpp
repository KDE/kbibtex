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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "onlinesearchoclcworldcat.h"

#include <KLocale>

class OnlineSearchOCLCWorldCat::Private
{
private:
    OnlineSearchOCLCWorldCat *p;

public:
    Private(OnlineSearchOCLCWorldCat *parent)
            : p(parent) {
        /// nothing
    }
};

OnlineSearchOCLCWorldCat::OnlineSearchOCLCWorldCat(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchOCLCWorldCat::Private(this)) {
    // TODO
}

OnlineSearchOCLCWorldCat::~OnlineSearchOCLCWorldCat() {
    delete d;
}

void OnlineSearchOCLCWorldCat::startSearch() {
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
    // TOOD
}

void OnlineSearchOCLCWorldCat::startSearch(const QMap<QString, QString> &query, int numResults) {
    Q_UNUSED(query)
    Q_UNUSED(numResults)
    m_hasBeenCanceled = false;
    delayedStoppedSearch(resultNoError);
    // TOOD
}

QString OnlineSearchOCLCWorldCat::label() const {
    return i18n("OCLC WorldCat");
}

OnlineSearchQueryFormAbstract *OnlineSearchOCLCWorldCat::customWidget(QWidget *) {
    return NULL;
}

KUrl OnlineSearchOCLCWorldCat::homepage() const {
    return KUrl(QLatin1String("http://www.worldcat.org/"));
}

void OnlineSearchOCLCWorldCat::cancel() {
    OnlineSearchAbstract::cancel();
}

QString OnlineSearchOCLCWorldCat::favIconUrl() const {
    return QLatin1String("http://www.worldcat.org/favicon.ico");
}
