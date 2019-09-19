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

#include "onlinesearchgeneral.h"

#ifdef HAVE_QTWIDGETS
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>

#include <KLocalizedString>
#include <KConfigGroup>

#include <KBibTeX>
#include <Entry>
#include "onlinesearchabstract_p.h"

namespace OnlineSearchGeneral {

#ifdef HAVE_QTWIDGETS
class Form::Private : public OnlineSearchAbstract::Form::Private
{
public:
    explicit Private(Form *parent)
            : OnlineSearchAbstract::Form::Private(),
          configGroupName(QStringLiteral("Search Engine General"))
    {
        QFormLayout *layout = new QFormLayout(parent);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("Free text:"), parent);
        QLineEdit *lineEdit = new QLineEdit(parent);
        layout->addRow(label, lineEdit);
        lineEdit->setClearButtonEnabled(true);
        lineEdit->setFocus(Qt::TabFocusReason);
        queryFields.insert(OnlineSearchAbstract::queryKeyFreeText, lineEdit);
        label->setBuddy(lineEdit);
        connect(lineEdit, &QLineEdit::returnPressed, parent, &OnlineSearchAbstract::Form::returnPressed);

        label = new QLabel(i18n("Title:"), parent);
        lineEdit = new QLineEdit(parent);
        layout->addRow(label, lineEdit);
        lineEdit->setClearButtonEnabled(true);
        queryFields.insert(OnlineSearchAbstract::queryKeyTitle, lineEdit);
        label->setBuddy(lineEdit);
        connect(lineEdit, &QLineEdit::returnPressed, parent, &OnlineSearchAbstract::Form::returnPressed);

        label = new QLabel(i18n("Author:"), parent);
        lineEdit = new QLineEdit(parent);
        layout->addRow(label, lineEdit);
        lineEdit->setClearButtonEnabled(true);
        queryFields.insert(OnlineSearchAbstract::queryKeyAuthor, lineEdit);
        label->setBuddy(lineEdit);
        connect(lineEdit, &QLineEdit::returnPressed, parent, &OnlineSearchAbstract::Form::returnPressed);

        label = new QLabel(i18n("Year:"), parent);
        lineEdit = new QLineEdit(parent);
        layout->addRow(label, lineEdit);
        lineEdit->setClearButtonEnabled(true);
        queryFields.insert(OnlineSearchAbstract::queryKeyYear, lineEdit);
        label->setBuddy(lineEdit);
        connect(lineEdit, &QLineEdit::returnPressed, parent, &OnlineSearchAbstract::Form::returnPressed);

        label = new QLabel(i18n("Number of Results:"), parent);
        numResultsField = new QSpinBox(parent);
        layout->addRow(label, numResultsField);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(20);
        label->setBuddy(numResultsField);

        loadState();
    }

    virtual ~Private() {
        /// nothing
    }

    QMap<QString, QLineEdit *> queryFields;
    QSpinBox *numResultsField;
    const QString configGroupName;

    void loadState()
    {
        KConfigGroup configGroup(config, configGroupName);
        for (QMap<QString, QLineEdit *>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
            it.value()->setText(configGroup.readEntry(it.key(), QString()));
        }
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
    }

    void saveState()
    {
        KConfigGroup configGroup(config, configGroupName);
        for (QMap<QString, QLineEdit *>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
            configGroup.writeEntry(it.key(), it.value()->text());
        }
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        config->sync();
    }
};

Form::Form(QWidget *parent)
        : OnlineSearchAbstract::Form(parent), dg(new OnlineSearchGeneral::Form::Private(this))
{
    /// nothing
}

Form::~Form()
{
    delete dg;
}

#endif // HAVE_QTWIDGETS


bool Form::readyToStart() const
{
    for (QMap<QString, QLineEdit *>::ConstIterator it = dg->queryFields.constBegin(); it != dg->queryFields.constEnd(); ++it)
        if (!it.value()->text().isEmpty())
            return true;

    return false;
}

void Form::copyFromEntry(const Entry &entry)
{
    dg->queryFields[OnlineSearchAbstract::queryKeyFreeText]->setText(d->guessFreeText(entry));
    dg->queryFields[OnlineSearchAbstract::queryKeyTitle]->setText(PlainTextValue::text(entry[Entry::ftTitle]));
    dg->queryFields[OnlineSearchAbstract::queryKeyAuthor]->setText(d->authorLastNames(entry).join(QStringLiteral(" ")));
    dg->queryFields[OnlineSearchAbstract::queryKeyYear]->setText(PlainTextValue::text(entry[Entry::ftYear]));
}

QMap<QString, QString> Form::getQueryTerms()
{
    QMap<QString, QString> result;

    for (QMap<QString, QLineEdit *>::ConstIterator it = dg->queryFields.constBegin(); it != dg->queryFields.constEnd(); ++it) {
        if (!it.value()->text().isEmpty())
            result.insert(it.key(), it.value()->text());
    }

    dg->saveState();
    return result;
}

int Form::getNumResults()
{
    return dg->numResultsField->value();
}

} /// namespace OnlineSearchGeneral

// #include "onlinesearchgeneral.moc"

#endif // HAVE_QTWIDGETS
