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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHGENERAL_H
#define KBIBTEX_NETWORKING_ONLINESEARCHGENERAL_H

#include <KSharedConfig>

#include <onlinesearch/OnlineSearchAbstract>

#ifdef HAVE_KF5
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF5

#ifdef HAVE_QTWIDGETS
class QSpinBox;
class QLineEdit;

class KBIBTEXNETWORKING_EXPORT OnlineSearchQueryFormGeneral : public OnlineSearchQueryFormAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchQueryFormGeneral(QWidget *parent);

    bool readyToStart() const override;
    void copyFromEntry(const Entry &) override;

    QMap<QString, QString> getQueryTerms();
    int getNumResults();

private:
    QMap<QString, QLineEdit *> queryFields;
    QSpinBox *numResultsField;
    const QString configGroupName;

    void loadState();
    void saveState();
};
#endif // HAVE_QTWIDGETS

#endif // KBIBTEX_NETWORKING_ONLINESEARCHGENERAL_H
