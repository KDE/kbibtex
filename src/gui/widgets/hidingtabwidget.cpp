/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#include <KDebug>

#include "hidingtabwidget.h"

/// required to for QSet<HiddenTabInfo>
uint qHash(const HidingTabWidget::HiddenTabInfo &hti)
{
    return qHash(hti.widget);
}

/// required to for QSet<HiddenTabInfo>
bool operator==(const HidingTabWidget::HiddenTabInfo &a, const HidingTabWidget::HiddenTabInfo &b)
{
    return a.widget == b.widget;
}

const int HidingTabWidget::InvalidTabPosition = -1;

HidingTabWidget::HidingTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    /// nothing to see here
}

QWidget *HidingTabWidget::hideTab(int index)
{
    if (index < 0 || index >= count()) return NULL;

    HiddenTabInfo hti;
    hti.widget = widget(index);
    hti.leftNeighborWidget = index > 0 ? widget(index - 1) : NULL;
    hti.rightNeighborWidget = index < count() - 1 ? widget(index + 1) : NULL;
    hti.label = tabText(index);
    hti.icon = tabIcon(index);
    hti.enabled = isTabEnabled(index);
    hti.toolTip = tabToolTip(index);
    hti.whatsThis = tabWhatsThis(index);
    m_hiddenTabInfo.insert(hti);

    QTabWidget::removeTab(index);

    return hti.widget;
}

int HidingTabWidget::showTab(QWidget *page)
{
    foreach(const HiddenTabInfo & hti, m_hiddenTabInfo) {
        if (hti.widget == page)
            return showTab(hti);
    }

    return InvalidTabPosition;
}

int HidingTabWidget::showTab(const HiddenTabInfo &hti, int index)
{
    if (index == InvalidTabPosition) {
        index = count(); ///< append at end of tab row
        int i = InvalidTabPosition;
        if ((i = indexOf(hti.leftNeighborWidget)) >= 0)
            index = i + 1; ///< right of left neighbor
        else if ((i = indexOf(hti.rightNeighborWidget)) >= 0)
            index = i; ///< left of right neighbor
    }

    /// insert tab using QTabWidget's original function
    index = QTabWidget::insertTab(index, hti.widget, hti.icon, hti.label);
    /// restore tab's properties
    setTabToolTip(index, hti.toolTip);
    setTabWhatsThis(index, hti.whatsThis);
    setTabEnabled(index, hti.enabled);

    return index;
}

void HidingTabWidget::removeTab(int index)
{
    if (index >= 0 && index < count()) {
        QWidget *page = widget(index);
        foreach(const HiddenTabInfo & hti, m_hiddenTabInfo) {
            if (hti.widget == page) {
                m_hiddenTabInfo.remove(hti);
                break;
            }
        }
        QTabWidget::removeTab(index);
    }
}

int HidingTabWidget::addTab(QWidget *page, const QString &label)
{
    foreach(const HiddenTabInfo & hti, m_hiddenTabInfo) {
        if (hti.widget == page) {
            int pos = showTab(hti);
            setTabText(pos, label);
            return pos;
        }
    }

    return QTabWidget::addTab(page, label);
}

int HidingTabWidget::addTab(QWidget *page, const QIcon &icon, const QString &label)
{
    foreach(const HiddenTabInfo & hti, m_hiddenTabInfo) {
        if (hti.widget == page) {
            int pos = showTab(hti);
            setTabIcon(pos, icon);
            setTabText(pos, label);
            return pos;
        }
    }

    return QTabWidget::addTab(page, icon, label);
}

int HidingTabWidget::insertTab(int index, QWidget *page, const QString &label)
{
    foreach(const HiddenTabInfo & hti, m_hiddenTabInfo) {
        if (hti.widget == page) {
            int pos = showTab(hti, index);
            setTabText(pos, label);
            return pos;
        }
    }

    return QTabWidget::insertTab(index, page, label);
}

int HidingTabWidget::insertTab(int index, QWidget *page, const QIcon &icon, const QString &label)
{
    foreach(const HiddenTabInfo & hti, m_hiddenTabInfo) {
        if (hti.widget == page) {
            index = showTab(hti, index);
            setTabIcon(index, icon);
            setTabText(index, label);
            return index;
        }
    }

    return QTabWidget::insertTab(index, page, icon, label);
}

