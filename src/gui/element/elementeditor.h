/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_GUI_ELEMENTEDITOR_H
#define KBIBTEX_GUI_ELEMENTEDITOR_H

#include <QWidget>

#include "kbibtexgui_export.h"

class Element;
class File;

/**
@author Thomas Fischer
 */
class KBIBTEXGUI_EXPORT ElementEditor : public QWidget
{
    Q_OBJECT
public:
    class ApplyElementInterface
    {
    public:
        virtual ~ApplyElementInterface() {
            /** nothing */
        }
        virtual void apply(QSharedPointer<Element>) = 0;
        virtual bool validate(QWidget **widgetWithIssue, QString &message) const = 0;
    };

    ElementEditor(bool scrollable, QWidget *parent);
    ~ElementEditor() override;

    void setElement(QSharedPointer<Element> element, const File *file);
    void setElement(QSharedPointer<const Element> element, const File *file);
    void setReadOnly(bool isReadOnly = true);
    bool elementChanged();
    bool elementUnapplied();
    bool validate(QWidget **widgetWithIssue, QString &message);

    QWidget *currentPage() const;
    void setCurrentPage(QWidget *tab);

Q_SIGNALS:
    void modified(bool);

public Q_SLOTS:
    void apply();
    void reset();
    bool validate();

private Q_SLOTS:
    void tabChanged();
    void switchToTab(const QString &tabIdentifier);
    void childModified(bool);

private:
    class ElementEditorPrivate;
    ElementEditorPrivate *d;
};

#endif // KBIBTEX_GUI_ELEMENTEDITOR_H
