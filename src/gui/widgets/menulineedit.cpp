/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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

#include <QLayout>
#include <QApplication>
#include <QTextEdit>

#include <KPushButton>
#include <KLineEdit>

#include "menulineedit.h"

class MenuLineEdit::MenuLineEditPrivate
{
private:
    MenuLineEdit *p;
    bool isMultiLine;
    QHBoxLayout *hLayout;

public:
    KPushButton *m_pushButtonType;
    KLineEdit *m_singleLineEditText;
    QTextEdit *m_multiLineEditText;

    MenuLineEditPrivate(bool isMultiLine, MenuLineEdit *parent)
            : p(parent), m_singleLineEditText(NULL), m_multiLineEditText(NULL) {
        this->isMultiLine = isMultiLine;
    }

    void setupUI() {
        p->setObjectName("FieldLineEdit");
        p->setFrameShape(QFrame::StyledPanel);

        hLayout = new QHBoxLayout(p);
        hLayout->setMargin(0);
        hLayout->setSpacing(2);

        m_pushButtonType = new KPushButton(p);
        hLayout->addWidget(m_pushButtonType);
        hLayout->setStretchFactor(m_pushButtonType, 0);
        m_pushButtonType->setObjectName("FieldLineEditButton");

        if (isMultiLine) {
            m_multiLineEditText = new QTextEdit(p);
            hLayout->addWidget(m_multiLineEditText);
            m_multiLineEditText->setObjectName("FieldLineEditText");
            connect(m_multiLineEditText->document(), SIGNAL(modificationChanged(bool)), p, SIGNAL(editingFinished()));
            m_multiLineEditText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        } else {
            m_singleLineEditText = new KLineEdit(p);
            hLayout->addWidget(m_singleLineEditText);
            hLayout->setStretchFactor(m_singleLineEditText, 100);
            m_singleLineEditText->setObjectName("FieldLineEditText");
            m_singleLineEditText->setClearButtonShown(true);
            connect(m_singleLineEditText, SIGNAL(editingFinished()), p, SIGNAL(editingFinished()));
            m_singleLineEditText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        }

        // qApp->setStyleSheet("QFrame#FieldLineEdit { background-color: " + QPalette().color(QPalette::Base).name() + "; } QTextEdit#FieldLineEditText { border-style: none; } KLineEdit#FieldLineEditText { border-style: none; } KPushButton#FieldLineEditButton { padding: 0px; margin-left:2px; margin-right:2px; text-align: left; background-color: " + QPalette().color(QPalette::Base).name() + "; border-style: none; } KPushButton#MonthSelector { padding: 0px; margin-left:2px; margin-right:2px; text-align: left; background-color: " + QPalette().color(QPalette::Base).name() + "; border-style: none; }");
        p->setStyleSheet(QLatin1String("background-color: ") + QPalette().color(QPalette::Base).name() + QLatin1String(";"));
        m_pushButtonType->setStyleSheet(QLatin1String("padding: 0px; margin-left:2px; margin-right:2px; text-align: left; background-color: ") + QPalette().color(QPalette::Base).name() + QLatin1String("; border-style: none;"));
        if (m_multiLineEditText != NULL)
            m_multiLineEditText->setStyleSheet(QLatin1String("border-style: none;"));
        if (m_singleLineEditText != NULL)
            m_singleLineEditText->setStyleSheet(QLatin1String("border-style: none;"));

        p->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    }

    void prependWidget(QWidget *widget) {
        widget->setParent(p);
        hLayout->insertWidget(0, widget);
    }

    void appendWidget(QWidget *widget) {
        widget->setParent(p);
        hLayout->addWidget(widget);
    }

};

MenuLineEdit::MenuLineEdit(bool isMultiLine, QWidget *parent)
        : QFrame(parent), d(new MenuLineEditPrivate(isMultiLine, this))
{
    d->setupUI();
}

void MenuLineEdit::setMenu(QMenu *menu)
{
    d->m_pushButtonType->setMenu(menu);
}

void MenuLineEdit::setReadOnly(bool readOnly)
{
    if (d->m_singleLineEditText != NULL)
        d->m_singleLineEditText->setReadOnly(readOnly);
    if (d->m_multiLineEditText != NULL)
        d->m_multiLineEditText->setReadOnly(readOnly);
}

QString MenuLineEdit::text() const
{
    if (d->m_singleLineEditText != NULL)
        return d->m_singleLineEditText->text();
    if (d->m_multiLineEditText != NULL)
        return d->m_multiLineEditText->document()->toPlainText();
    return QLatin1String("");
}

void MenuLineEdit::setText(const QString &text)
{
    if (d->m_singleLineEditText != NULL)
        d->m_singleLineEditText->setText(text);
    if (d->m_multiLineEditText != NULL)
        d->m_multiLineEditText->document()->setPlainText(text);
}

void MenuLineEdit::setIcon(const KIcon & icon)
{
    d->m_pushButtonType->setIcon(icon);
}

void MenuLineEdit::setFont(const QFont & font)
{
    if (d->m_singleLineEditText != NULL)
        d->m_singleLineEditText->setFont(font);
    if (d->m_multiLineEditText != NULL)
        d->m_multiLineEditText->document()->setDefaultFont(font);
}

void MenuLineEdit::setButtonToolTip(const QString &text)
{
    d->m_pushButtonType->setToolTip(text);
}

void MenuLineEdit::prependWidget(QWidget *widget)
{
    d->prependWidget(widget);
}

void MenuLineEdit::appendWidget(QWidget *widget)
{
    d->appendWidget(widget);
}

bool MenuLineEdit::isModified() const
{
    if (d->m_singleLineEditText != NULL)
        return d->m_singleLineEditText->isModified();
    if (d->m_multiLineEditText != NULL)
        return d->m_multiLineEditText->document()->isModified();
    return false;
}
