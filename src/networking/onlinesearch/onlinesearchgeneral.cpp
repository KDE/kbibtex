/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
        queryFields.insert(OnlineSearchAbstract::QueryKey::FreeText, lineEdit);
        label->setBuddy(lineEdit);
        connect(lineEdit, &QLineEdit::returnPressed, parent, &OnlineSearchAbstract::Form::returnPressed);

        label = new QLabel(i18n("Title:"), parent);
        lineEdit = new QLineEdit(parent);
        layout->addRow(label, lineEdit);
        lineEdit->setClearButtonEnabled(true);
        queryFields.insert(OnlineSearchAbstract::QueryKey::Title, lineEdit);
        label->setBuddy(lineEdit);
        connect(lineEdit, &QLineEdit::returnPressed, parent, &OnlineSearchAbstract::Form::returnPressed);

        label = new QLabel(i18n("Author:"), parent);
        lineEdit = new QLineEdit(parent);
        layout->addRow(label, lineEdit);
        lineEdit->setClearButtonEnabled(true);
        queryFields.insert(OnlineSearchAbstract::QueryKey::Author, lineEdit);
        label->setBuddy(lineEdit);
        connect(lineEdit, &QLineEdit::returnPressed, parent, &OnlineSearchAbstract::Form::returnPressed);

        label = new QLabel(i18n("Year:"), parent);
        lineEdit = new QLineEdit(parent);
        layout->addRow(label, lineEdit);
        lineEdit->setClearButtonEnabled(true);
        queryFields.insert(OnlineSearchAbstract::QueryKey::Year, lineEdit);
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

    QMap<OnlineSearchAbstract::QueryKey, QLineEdit *> queryFields;
    QSpinBox *numResultsField;
    const QString configGroupName;

/// GCC's C++ compiler may complain about that the following function does reach
/// its end without returning anything. Seemingly, the compiler is not aware that
/// the switch covers all cases of the OnlineSearchAbstract::QueryKey enum.
/// Unfortunately, this problem causes the C++ compiler inside GitLab's CI to stop
/// and issue an error, making the CI to abort.
/// Thus, this particular check gets temporarily disabled.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
    static QString queryKeyToString(const OnlineSearchAbstract::QueryKey &queryKey)
    {
        switch (queryKey) {
        case OnlineSearchAbstract::QueryKey::FreeText: return QStringLiteral("free");
        case OnlineSearchAbstract::QueryKey::Title: return QStringLiteral("title");
        case OnlineSearchAbstract::QueryKey::Author: return QStringLiteral("author");
        case OnlineSearchAbstract::QueryKey::Year: return QStringLiteral("year");
        }
    }
#pragma GCC diagnostic pop

    void loadState()
    {
        KConfigGroup configGroup(config, configGroupName);
        for (QMap<OnlineSearchAbstract::QueryKey, QLineEdit *>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
            it.value()->setText(configGroup.readEntry(queryKeyToString(it.key()), QString()));
        }
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
    }

    void saveState()
    {
        KConfigGroup configGroup(config, configGroupName);
        for (QMap<OnlineSearchAbstract::QueryKey, QLineEdit *>::ConstIterator it = queryFields.constBegin(); it != queryFields.constEnd(); ++it) {
            configGroup.writeEntry(queryKeyToString(it.key()), it.value()->text());
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
    for (QMap<OnlineSearchAbstract::QueryKey, QLineEdit *>::ConstIterator it = dg->queryFields.constBegin(); it != dg->queryFields.constEnd(); ++it)
        if (!it.value()->text().isEmpty())
            return true;

    return false;
}

void Form::copyFromEntry(const Entry &entry)
{
    dg->queryFields[OnlineSearchAbstract::QueryKey::FreeText]->setText(d->guessFreeText(entry));
    dg->queryFields[OnlineSearchAbstract::QueryKey::Title]->setText(PlainTextValue::text(entry[Entry::ftTitle]));
    dg->queryFields[OnlineSearchAbstract::QueryKey::Author]->setText(d->authorLastNames(entry).join(QStringLiteral(" ")));
    dg->queryFields[OnlineSearchAbstract::QueryKey::Year]->setText(PlainTextValue::text(entry[Entry::ftYear]));
}

QMap<OnlineSearchAbstract::QueryKey, QString> Form::getQueryTerms()
{
    QMap<OnlineSearchAbstract::QueryKey, QString> result;

    for (QMap<OnlineSearchAbstract::QueryKey, QLineEdit *>::ConstIterator it = dg->queryFields.constBegin(); it != dg->queryFields.constEnd(); ++it) {
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
