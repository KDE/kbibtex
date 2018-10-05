/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "menulineedit.h"

#include <QLayout>
#include <QApplication>
#include <QMenu>
#include <QTimer>
#include <QPushButton>

#include <KLineEdit>
#include <KTextEdit>
#include <KConfigGroup>
#include <KSharedConfig>

#include "notificationhub.h"

const int MenuLineEdit::MenuLineConfigurationChangedEvent = NotificationHub::EventUserDefined + 1861;
const QString MenuLineEdit::keyLimitKeyboardTabStops = QStringLiteral("LimitKeyboardTabStops");

class MenuLineEdit::MenuLineEditPrivate : public NotificationListener
{
private:
    MenuLineEdit *p;
    bool isMultiLine;
    bool m_isReadOnly;
    QHBoxLayout *hLayout;
    static const QString transparentStyleSheet, normalStyleSheet;
    bool makeInnerWidgetsTransparent;

public:
    QPushButton *m_pushButtonType;
    KLineEdit *m_singleLineEditText;
    KTextEdit *m_multiLineEditText;

    MenuLineEditPrivate(bool isMultiLine, MenuLineEdit *parent)
            : p(parent), m_isReadOnly(false), makeInnerWidgetsTransparent(false), m_singleLineEditText(nullptr), m_multiLineEditText(nullptr) {
        this->isMultiLine = isMultiLine;
        /// listen to configuration change events specifically concerning a MenuLineEdit widget
        NotificationHub::registerNotificationListener(this, MenuLineEdit::MenuLineConfigurationChangedEvent);

        setupUI();
    }

    ~MenuLineEditPrivate() override {
        for (int i = hLayout->count() - 1; i >= 0; --i) {
            QWidget *w = hLayout->itemAt(i)->widget();
            w->deleteLater();
        }
    }

    void notificationEvent(int eventId) override {
        if (eventId == MenuLineEdit::MenuLineConfigurationChangedEvent) {
            /// load setting limitKeyboardTabStops
            KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
            static QString const configGroupName = QStringLiteral("User Interface");
            KConfigGroup configGroup(config, configGroupName);
            const bool limitKeyboardTabStops = configGroup.readEntry(MenuLineEdit::keyLimitKeyboardTabStops, false);

            /// check each widget inside MenuLineEdit
            for (int i = hLayout->count() - 1; i >= 0; --i) {
                QWidget *w = hLayout->itemAt(i)->widget();
                if (w != nullptr && w != m_singleLineEditText && w != m_multiLineEditText) {
                    /// for all widgets except the main editing widget: change tab focus policy
                    w->setFocusPolicy(limitKeyboardTabStops ? Qt::ClickFocus : Qt::StrongFocus);
                }
            }
        }
    }

    void setupUI() {
        p->setObjectName(QStringLiteral("FieldLineEdit"));

        hLayout = new QHBoxLayout(p);
        hLayout->setMargin(0);
        hLayout->setSpacing(2);

        m_pushButtonType = new QPushButton(p);
        appendWidget(m_pushButtonType);
        hLayout->setStretchFactor(m_pushButtonType, 0);
        m_pushButtonType->setObjectName(QStringLiteral("FieldLineEditButton"));

        if (isMultiLine) {
            m_multiLineEditText = new KTextEdit(p);
            appendWidget(m_multiLineEditText);
            connect(m_multiLineEditText, &KTextEdit::textChanged, p, &MenuLineEdit::slotTextChanged);
            m_multiLineEditText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            p->setFocusProxy(m_multiLineEditText);
            m_multiLineEditText->setAcceptRichText(false);
        } else {
            m_singleLineEditText = new KLineEdit(p);
            appendWidget(m_singleLineEditText);
            hLayout->setStretchFactor(m_singleLineEditText, 100);
            m_singleLineEditText->setClearButtonEnabled(true);
            m_singleLineEditText->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
            m_singleLineEditText->setCompletionMode(KCompletion::CompletionPopup);
            m_singleLineEditText->completionObject()->setIgnoreCase(true);
            p->setFocusProxy(m_singleLineEditText);
            connect(m_singleLineEditText, &KLineEdit::textEdited, p, &MenuLineEdit::textChanged);
        }

        p->setFocusPolicy(Qt::StrongFocus); // FIXME improve focus handling
        p->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    }

    void prependWidget(QWidget *widget) {
        widget->setParent(p);
        hLayout->insertWidget(0, widget);
        widget->setStyleSheet(makeInnerWidgetsTransparent ? transparentStyleSheet : normalStyleSheet);
        setWidgetReadOnly(widget, m_isReadOnly);
        fixTabOrder();
    }

    void appendWidget(QWidget *widget) {
        widget->setParent(p);
        hLayout->addWidget(widget);
        widget->setStyleSheet(makeInnerWidgetsTransparent ? transparentStyleSheet : normalStyleSheet);
        setWidgetReadOnly(widget, m_isReadOnly);
        fixTabOrder();
    }

    void fixTabOrder() {
        QWidget *cur = nullptr;
        if (hLayout->count() > 0)
            p->setTabOrder(p, (cur = hLayout->itemAt(0)->widget()));
        for (int i = 1; i < hLayout->count(); ++i) {
            QWidget *next = hLayout->itemAt(i)->widget();
            p->setTabOrder(cur, next);
            cur = next;
        }
    }

    void verticallyStretchButtons() {
        /// do not vertically stretch if using transparent style sheet
        if (makeInnerWidgetsTransparent) return;

        /// check each widget inside MenuLineEdit
        for (int i = hLayout->count() - 1; i >= 0; --i) {
            QWidget *w = hLayout->itemAt(i)->widget();
            if (w != nullptr && w != m_singleLineEditText && w != m_multiLineEditText) {
                /// for all widgets except the main editing widget: change tab focus policy
                QSizePolicy sp = w->sizePolicy();
                w->setSizePolicy(sp.horizontalPolicy(), QSizePolicy::MinimumExpanding);
            }
        }
    }

    void setStyleSheet(bool makeInnerWidgetsTransparent) {
        this->makeInnerWidgetsTransparent = makeInnerWidgetsTransparent;
        for (int i = hLayout->count() - 1; i >= 0; --i) {
            QWidget *w = hLayout->itemAt(i)->widget();
            if (w != nullptr)
                w->setStyleSheet(makeInnerWidgetsTransparent ? transparentStyleSheet : normalStyleSheet);
        }
    }

    void setWidgetReadOnly(QWidget *w, bool isReadOnly) {
        if (m_singleLineEditText == w)
            m_singleLineEditText->setReadOnly(isReadOnly);
        else if (m_multiLineEditText == w)
            m_multiLineEditText->setReadOnly(isReadOnly);
        else if (!w->property("isConst").isValid() && !w->property("isConst").toBool())
            w->setEnabled(!isReadOnly);
    }

    void setReadOnly(bool isReadOnly) {
        m_isReadOnly = isReadOnly;
        for (int i = hLayout->count() - 1; i >= 0; --i) {
            QWidget *w = hLayout->itemAt(i)->widget();
            setWidgetReadOnly(w, isReadOnly);
        }
    }
};

const QString MenuLineEdit::MenuLineEditPrivate::transparentStyleSheet = QStringLiteral("KTextEdit { border-style: none; background-color: transparent; }\nKLineEdit { border-style: none; background-color: transparent; }\nKPushButton { border-style: none; background-color: transparent; padding: 0px; margin-left: 2px; margin-right:2px; text-align: left; }");
const QString MenuLineEdit::MenuLineEditPrivate::normalStyleSheet = QStringLiteral("QPushButton { padding:4px; margin:0px;  text-align: left; }\nQPushButton::menu-indicator {subcontrol-position: right center; subcontrol-origin: content;}");

MenuLineEdit::MenuLineEdit(bool isMultiLine, QWidget *parent)
        : QFrame(parent), d(new MenuLineEditPrivate(isMultiLine, this))
{
    if (d->m_singleLineEditText != nullptr) {
        /// Only for single-line variants stretch buttons vertically
        QTimer::singleShot(250, this, &MenuLineEdit::slotVerticallyStretchButtons);
    }
}

MenuLineEdit::~MenuLineEdit()
{
    delete d;
}

void MenuLineEdit::setMenu(QMenu *menu)
{
    d->m_pushButtonType->setMenu(menu);
}

void MenuLineEdit::setReadOnly(bool readOnly)
{
    d->setReadOnly(readOnly);
}

QString MenuLineEdit::text() const
{
    if (d->m_singleLineEditText != nullptr)
        return d->m_singleLineEditText->text();
    if (d->m_multiLineEditText != nullptr)
        return d->m_multiLineEditText->document()->toPlainText();
    return QString();
}

void MenuLineEdit::setText(const QString &text)
{
    if (d->m_singleLineEditText != nullptr) {
        d->m_singleLineEditText->setText(text);
        d->m_singleLineEditText->setCursorPosition(0);
    } else if (d->m_multiLineEditText != nullptr) {
        d->m_multiLineEditText->document()->setPlainText(text);
        QTextCursor tc = d->m_multiLineEditText->textCursor();
        tc.setPosition(0);
        d->m_multiLineEditText->setTextCursor(tc);
    }
}

void MenuLineEdit::setIcon(const QIcon &icon)
{
    d->m_pushButtonType->setIcon(icon);
}

void MenuLineEdit::setFont(const QFont &font)
{
    if (d->m_singleLineEditText != nullptr)
        d->m_singleLineEditText->setFont(font);
    if (d->m_multiLineEditText != nullptr)
        d->m_multiLineEditText->document()->setDefaultFont(font);
}

void MenuLineEdit::setButtonToolTip(const QString &text)
{
    d->m_pushButtonType->setToolTip(text);
}

void MenuLineEdit::setChildAcceptDrops(bool acceptDrops)
{
    if (d->m_singleLineEditText != nullptr)
        d->m_singleLineEditText->setAcceptDrops(acceptDrops);
    if (d->m_multiLineEditText != nullptr)
        d->m_multiLineEditText->setAcceptDrops(acceptDrops);
}

QWidget *MenuLineEdit::buddy()
{
    if (d->m_singleLineEditText != nullptr)
        return d->m_singleLineEditText;
    if (d->m_multiLineEditText != nullptr)
        return d->m_multiLineEditText;
    return nullptr;
}

void MenuLineEdit::prependWidget(QWidget *widget)
{
    d->prependWidget(widget);
}

void MenuLineEdit::appendWidget(QWidget *widget)
{
    d->appendWidget(widget);
}

void MenuLineEdit::setInnerWidgetsTransparency(bool makeInnerWidgetsTransparent)
{
    d->setStyleSheet(makeInnerWidgetsTransparent);
}

bool MenuLineEdit::isModified() const
{
    if (d->m_singleLineEditText != nullptr)
        return d->m_singleLineEditText->isModified();
    if (d->m_multiLineEditText != nullptr)
        return d->m_multiLineEditText->document()->isModified();
    return false;
}

void MenuLineEdit::setCompletionItems(const QStringList &items)
{
    if (d->m_singleLineEditText != nullptr)
        d->m_singleLineEditText->completionObject()->setItems(items);
}

void MenuLineEdit::focusInEvent(QFocusEvent *)
{
    if (d->m_singleLineEditText != nullptr)
        d->m_singleLineEditText->setFocus();
    else if (d->m_multiLineEditText != nullptr)
        d->m_multiLineEditText->setFocus();
}

void MenuLineEdit::slotTextChanged()
{
    Q_ASSERT_X(d->m_multiLineEditText != nullptr, "MenuLineEdit::slotTextChanged", "d->m_multiLineEditText is NULL");
    emit textChanged(d->m_multiLineEditText->toPlainText());
}

void MenuLineEdit::slotVerticallyStretchButtons()
{
    d->verticallyStretchButtons();
}
