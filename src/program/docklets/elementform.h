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

#ifndef KBIBTEX_PROGRAM_ELEMENTFORM_H
#define KBIBTEX_PROGRAM_ELEMENTFORM_H


#include <QWidget>

class QDockWidget;

class MDIWidget;
class Element;
class File;

class ElementForm : public QWidget
{
    Q_OBJECT

public:
    ElementForm(MDIWidget *mdiWidget, QDockWidget *parent);
    ~ElementForm() override;

public slots:
    void setElement(QSharedPointer<Element>, const File *);
    void refreshElement();

signals:
    void elementModified();

private:
    class ElementFormPrivate;
    ElementFormPrivate *d;

private slots:
    void modified(bool);
    void apply();
    bool validateAndOnlyThenApply();
    void reset();
    void visibilityChanged(bool);
    void autoApplyToggled(bool);
};

#endif // KBIBTEX_PROGRAM_ELEMENTFORM_H
