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

#ifndef KBIBTEX_GUI_HIDINGTABWIDGET_H
#define KBIBTEX_GUI_HIDINGTABWIDGET_H

#include <QTabWidget>

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
    Q_OBJECT

public:
    /// Negative value to describe an invalid tab position
    static const int InvalidTabPosition;

    explicit HidingTabWidget(QWidget *parent = nullptr);
    ~HidingTabWidget();

    /**
     * Hides the tab at position @param index from this stack of widgets.
     * The page widget itself is not deleted.
     * For future reference, the page widget is returned.
     * If @param index does not refer to a valid widget, nullptr will be returned.
     */
    QWidget *hideTab(int index);

    /**
     * Shows a previously hidden tab reusing its properties (label, icon, tooltip, what's this, enbled).
     * If possible, the hidden tab will be shown between its previous neighbor tabs.
     * If the provided @param page was not added by @see addTab or @see insertTab,
     * the function will return InvalidTabPosition. If the tab could be shown again, its new position will be returned.
     * The behavior is undefined if @param page is not a value returned by a previous @see hideTab call.
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

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_GUI_HIDINGTABWIDGET_H
