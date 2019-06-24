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

#include "hidingtabwidget.h"

#include <QSet>

typedef struct {
    /// the hidden widget
    QWidget *widget;
    /// the hidden widget's neighboring widgets,
    /// used to find a place where to insert the tab when it will be shown again.
    QWidget *leftNeighborWidget, *rightNeighborWidget;
    /// tab properties
    QIcon icon;
    QString label;
    bool enabled;
    QString toolTip;
    QString whatsThis;
} HiddenTabInfo;

class HidingTabWidget::Private {
private:
    HidingTabWidget *parent;
    QTabWidget *parentAsQTabWidget;

public:
    QSet<HiddenTabInfo> hiddenTabInfo;

    Private(HidingTabWidget *_parent)
            : parent(_parent), parentAsQTabWidget(qobject_cast<QTabWidget *>(_parent))
    {
        /// nothing
    }

    int showTab(const HiddenTabInfo &hti, int index = InvalidTabPosition)
    {
        if (index <0) {
            /// No position to insert tab given, so make an educated guess
            index = parent->count(); ///< Append at end of tab row by default
            int i = InvalidTabPosition;
            if ((i = parent->indexOf(hti.leftNeighborWidget)) >= 0)
                index = i + 1; ///< Right of left neighbor
            else if ((i = parent->indexOf(hti.rightNeighborWidget)) >= 0)
                index = i; ///< Left of right neighbor
        }

        /// Insert tab using QTabWidget's original function
        index = parentAsQTabWidget->insertTab(index, hti.widget, hti.icon, hti.label);
        /// Restore tab's properties
        parent->setTabToolTip(index, hti.toolTip);
        parent->setTabWhatsThis(index, hti.whatsThis);
        parent->setTabEnabled(index, hti.enabled);

        return index;
    }
};


/// required to for QSet<HiddenTabInfo>
uint qHash(const HiddenTabInfo &hti)
{
    return qHash(hti.widget);
}

/// required to for QSet<HiddenTabInfo>
bool operator==(const HiddenTabInfo &a, const HiddenTabInfo &b)
{
    return a.widget == b.widget;
}

const int HidingTabWidget::InvalidTabPosition = -1;

HidingTabWidget::HidingTabWidget(QWidget *parent)
        : QTabWidget(parent), d(new Private(this))
{
    /// nothing to see here
}

HidingTabWidget::~HidingTabWidget()
{
    delete d;
}

QWidget *HidingTabWidget::hideTab(int index)
{
    if (index < 0 || index >= count()) return nullptr;

    HiddenTabInfo hti;
    hti.widget = widget(index);
    hti.leftNeighborWidget = index > 0 ? widget(index - 1) : nullptr;
    hti.rightNeighborWidget = index < count() - 1 ? widget(index + 1) : nullptr;
    hti.label = tabText(index);
    hti.icon = tabIcon(index);
    hti.enabled = isTabEnabled(index);
    hti.toolTip = tabToolTip(index);
    hti.whatsThis = tabWhatsThis(index);
    d->hiddenTabInfo.insert(hti);

    QTabWidget::removeTab(index);

    return hti.widget;
}

int HidingTabWidget::showTab(QWidget *page)
{
    for (const HiddenTabInfo &hti : const_cast<const QSet<HiddenTabInfo> &>(d->hiddenTabInfo)) {
        if (hti.widget == page)
            return d->showTab(hti);
    }

    return InvalidTabPosition;
}

void HidingTabWidget::removeTab(int index)
{
    if (index >= 0 && index < count()) {
        QWidget *page = widget(index);
        for (const HiddenTabInfo &hti : const_cast<const QSet<HiddenTabInfo> &>(d->hiddenTabInfo)) {
            if (hti.widget == page) {
                d->hiddenTabInfo.remove(hti);
                break;
            }
        }
        QTabWidget::removeTab(index);
    }
}

int HidingTabWidget::addTab(QWidget *page, const QString &label)
{
    for (const HiddenTabInfo &hti : const_cast<const QSet<HiddenTabInfo> &>(d->hiddenTabInfo)) {
        if (hti.widget == page) {
            int pos = d->showTab(hti);
            setTabText(pos, label);
            return pos;
        }
    }

    return QTabWidget::addTab(page, label);
}

int HidingTabWidget::addTab(QWidget *page, const QIcon &icon, const QString &label)
{
    for (const HiddenTabInfo &hti : const_cast<const QSet<HiddenTabInfo> &>(d->hiddenTabInfo)) {
        if (hti.widget == page) {
            int pos = d->showTab(hti);
            setTabIcon(pos, icon);
            setTabText(pos, label);
            return pos;
        }
    }

    return QTabWidget::addTab(page, icon, label);
}

int HidingTabWidget::insertTab(int index, QWidget *page, const QString &label)
{
    for (const HiddenTabInfo &hti : const_cast<const QSet<HiddenTabInfo> &>(d->hiddenTabInfo)) {
        if (hti.widget == page) {
            int pos = d->showTab(hti, index);
            setTabText(pos, label);
            return pos;
        }
    }

    return QTabWidget::insertTab(index, page, label);
}

int HidingTabWidget::insertTab(int index, QWidget *page, const QIcon &icon, const QString &label)
{
    for (const HiddenTabInfo &hti : const_cast<const QSet<HiddenTabInfo> &>(d->hiddenTabInfo)) {
        if (hti.widget == page) {
            index = d->showTab(hti, index);
            setTabIcon(index, icon);
            setTabText(index, label);
            return index;
        }
    }

    return QTabWidget::insertTab(index, page, icon, label);
}
