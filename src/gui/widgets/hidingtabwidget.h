/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#ifndef KBIBTEX_GUI_HIDINGTABWIDGET_H
#define KBIBTEX_GUI_HIDINGTABWIDGET_H

#include <QTabWidget>
#include <QSet>

/**
 * @brief The HidingTabWidget class to hide and show tabs in a QTabWidget.
 *
 * This class extends the original QTabWidget by the feature of hiding and showing
 * tabs previously added or inserted.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class HidingTabWidget : public QTabWidget
{
public:
    /// Negative value to describe an invalid tab position
    static const int InvalidTabPosition;

    HidingTabWidget(QWidget *parent = NULL);

    /**
     * Hides the tab at position @param index from this stack of widgets.
     * The page widget itself is not deleted.
     * For future reference, the page widget is returned.
     * If @param index does not refer to a valid widget, NULL will be returned.
     */
    QWidget *hideTab(int index);

    /**
     * Shows a previously hidden tab reusing its properties (label, icon, tooltip, what's this, enbled).
     * If possible, the hidden tab will be shown between its previous neighbor tabs.
     * If the provided @param page was not added by @see addTab or @see insertTab,
     * the function will return InvalidTabPosition. If the tab could be shown again, its new position will be returned.
     */
    int showTab(QWidget *page);

    /**
     * Reimplemented from QTabWidget, same semantics.
      */
    void removeTab(int index);

    /**
     * Reimplemented from QTabWidget, same semantics.
     */
    int addTab(QWidget *page, const QString &label);

    /**
     * Reimplemented from QTabWidget, same semantics.
     */
    int addTab(QWidget *page, const QIcon &icon, const QString &label);

    /**
     * Reimplemented from QTabWidget, same semantics.
     */
    int insertTab(int index, QWidget *page, const QString &label);

    /**
     * Reimplemented from QTabWidget, same semantics.
     */
    int insertTab(int index, QWidget *page, const QIcon &icon, const QString &label);

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

private:
    QSet<HiddenTabInfo> m_hiddenTabInfo;

    int showTab(const HiddenTabInfo &hti, int index = InvalidTabPosition);
};

#endif // KBIBTEX_GUI_HIDINGTABWIDGET_H
