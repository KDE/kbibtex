/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#include <QMenu>

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
        // FIXME: proper frame with colored shadows when focused
        p->setFrameShape(QFrame::StyledPanel);
        p->setFrameShadow(QFrame::Sunken);

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
            connect(m_multiLineEditText, SIGNAL(textChanged()), p, SLOT(slotTextChanged()));
            m_multiLineEditText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            p->setFocusProxy(m_multiLineEditText);
        } else {
            m_singleLineEditText = new KLineEdit(p);
            hLayout->addWidget(m_singleLineEditText);
            hLayout->setStretchFactor(m_singleLineEditText, 100);
            m_singleLineEditText->setClearButtonShown(true);
            connect(m_singleLineEditText, SIGNAL(textChanged(QString)), p, SIGNAL(textChanged(QString)));
            m_singleLineEditText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            p->setFocusProxy(m_singleLineEditText);
        }

        p->setFocusPolicy(Qt::StrongFocus);
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
    if (d->m_singleLineEditText != NULL) {
        d->m_singleLineEditText->setText(text);
        d->m_singleLineEditText->setCursorPosition(0);
    } else if (d->m_multiLineEditText != NULL) {
        d->m_multiLineEditText->document()->setPlainText(text);
        QTextCursor tc = d->m_multiLineEditText->textCursor();
        tc.setPosition(0);
        d->m_multiLineEditText->setTextCursor(tc);
    }
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

void MenuLineEdit::slotTextChanged()
{
    Q_ASSERT(d->m_multiLineEditText != NULL);
    emit textChanged(d->m_multiLineEditText->toPlainText());
}
