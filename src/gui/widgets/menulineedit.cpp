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

#include <KPushButton>
#include <KLineEdit>

#include "menulineedit.h"

using namespace KBibTeX::GUI::Widgets;

class MenuLineEdit::MenuLineEditPrivate
{
private:
    MenuLineEdit *p;

public:
    MenuLineEditPrivate(MenuLineEdit *parent) {
        p = parent;
    }

    KPushButton *m_pushButtonType;
    KLineEdit *m_lineEditText;

    void setupUI() {
        p->setObjectName("FieldLineEdit");
        p->setFrameShape(QFrame::StyledPanel);

        QHBoxLayout *hLayout = new QHBoxLayout(p);
        hLayout->setMargin(0);
        hLayout->setSpacing(2);

        m_pushButtonType = new KPushButton(p);
        hLayout->addWidget(m_pushButtonType);
        m_pushButtonType->setObjectName("FieldLineEditButton");

        m_lineEditText = new KLineEdit(p);
        hLayout->addWidget(m_lineEditText);
        m_lineEditText->setObjectName("FieldLineEditText");
        m_lineEditText->setClearButtonShown(true);

        qApp->setStyleSheet("QFrame#FieldLineEdit { background-color: " + QPalette().color(QPalette::Base).name() + "; } KLineEdit#FieldLineEditText { border-style: none; } KPushButton#FieldLineEditButton { padding: 0px; margin-left:2px; margin-right:2px; text-align: left; width: " + QString::number(m_pushButtonType->height() + 1) + "; background-color: " + QPalette().color(QPalette::Base).name() + "; border-style: none; }");

        p->setSizePolicy(m_lineEditText->sizePolicy());
    }

};

MenuLineEdit::MenuLineEdit(QWidget *parent)
        : QFrame(parent), d(new MenuLineEditPrivate(this))
{
    d->setupUI();
}

void MenuLineEdit::setMenu(QMenu *menu)
{
    d->m_pushButtonType->setMenu(menu);
}

void MenuLineEdit::setReadOnly(bool readOnly)
{
    d->m_lineEditText->setReadOnly(readOnly);
}

void MenuLineEdit::setText(const QString &text)
{
    d->m_lineEditText->setText(text);
}

void MenuLineEdit::setIcon(const KIcon & icon)
{
    d->m_pushButtonType->setIcon(icon);
}

void MenuLineEdit::setButtonToolTip(const QString &text)
{
    d->m_pushButtonType->setToolTip(text);
}
