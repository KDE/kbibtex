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

#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

#include <KLineEdit>
#include <KLocale>
#include <KConfigGroup>

#include "websearchgeneral.h"

WebSearchQueryFormGeneral::WebSearchQueryFormGeneral(QWidget *parent)
        : WebSearchQueryFormAbstract(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))),
        configGroupName(QLatin1String("Search Engines General"))
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);

    QLabel *label = new QLabel(i18n("Free text:"), this);
    layout->addWidget(label, 0, 0, 1, 1);
    KLineEdit *lineEdit = new KLineEdit(this);
    lineEdit->setClearButtonShown(true);
    lineEdit->setFocus(Qt::TabFocusReason);
    layout->addWidget(lineEdit, 0, 1, 1, 1);
    queryFields.insert(WebSearchAbstract::queryKeyFreeText, lineEdit);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Title:"), this);
    layout->addWidget(label, 1, 0, 1, 1);
    lineEdit = new KLineEdit(this);
    lineEdit->setClearButtonShown(true);
    queryFields.insert(WebSearchAbstract::queryKeyTitle, lineEdit);
    layout->addWidget(lineEdit, 1, 1, 1, 1);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Author:"), this);
    layout->addWidget(label, 2, 0, 1, 1);
    lineEdit = new KLineEdit(this);
    lineEdit->setClearButtonShown(true);
    queryFields.insert(WebSearchAbstract::queryKeyAuthor, lineEdit);
    layout->addWidget(lineEdit, 2, 1, 1, 1);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Year:"), this);
    layout->addWidget(label, 3, 0, 1, 1);
    lineEdit = new KLineEdit(this);
    lineEdit->setClearButtonShown(true);
    queryFields.insert(WebSearchAbstract::queryKeyYear, lineEdit);
    layout->addWidget(lineEdit, 3, 1, 1, 1);
    label->setBuddy(lineEdit);
    connect(lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    label = new QLabel(i18n("Number of Results:"), this);
    layout->addWidget(label, 4, 0, 1, 1);
    numResultsField = new QSpinBox(this);
    numResultsField->setMinimum(3);
    numResultsField->setMaximum(100);
    numResultsField->setValue(20);
    layout->addWidget(numResultsField, 4, 1, 1, 1);
    label->setBuddy(numResultsField);

    layout->setRowStretch(5, 100);

    loadState();
}

bool WebSearchQueryFormGeneral::readyToStart() const
{
    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it)
        if (!it.value()->text().isEmpty())
            return true;

    return false;
}

QMap<QString, QString> WebSearchQueryFormGeneral::getQueryTerms()
{
    QMap<QString, QString> result;

    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
        if (!it.value()->text().isEmpty())
            result.insert(it.key(), it.value()->text());
    }

    saveState();
    return result;
}

int WebSearchQueryFormGeneral::getNumResults()
{
    return numResultsField->value();
}

void WebSearchQueryFormGeneral::loadState()
{
    KConfigGroup configGroup(config, configGroupName);
    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
        it.value()->setText(configGroup.readEntry(it.key(), QString()));
    }
    numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
}

void WebSearchQueryFormGeneral::saveState()
{
    KConfigGroup configGroup(config, configGroupName);
    for (QMap<QString, KLineEdit*>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
        configGroup.writeEntry(it.key(), it.value()->text());
    }
    configGroup.writeEntry(QLatin1String("numResults"), numResultsField->value());
    config->sync();
}
