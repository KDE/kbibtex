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

#include <QList>
#include <QPair>
#include <QLabel>
#include <QApplication>

#include <KDebug>
#include <kmimetypetrader.h>
#include <kparts/part.h>
#include <kio/netaccess.h>

#include "mdiwidget.h"
#include "openfileinfo.h"

using namespace KBibTeX::Program;

class MDIWidget::MDIWidgetPrivate
{
public:
    MDIWidget *p;

    MDIWidgetPrivate(MDIWidget *parent)
            : p(parent) {
        // nothing
    }
};

MDIWidget::MDIWidget(QWidget *parent)
        : QStackedWidget(parent), d(new MDIWidgetPrivate(this))
{
    QLabel *label = new QLabel(i18n("<qt>Welcome to <b>KBibTeX</b> for <b>KDE 4</b><br/><br/>Please select a file to open</qt>"), this);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    addWidget(label);

}

void MDIWidget::setFile(OpenFileInfo *openFileInfo)
{
    KBibTeX::GUI::BibTeXEditor *oldEditor = NULL;
    bool hasChanged = true;
    QWidget *widget = openFileInfo->part(this)->widget();
    if (indexOf(widget) >= 0) {
        oldEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(currentWidget());
        hasChanged = widget != currentWidget();
    } else {
        addWidget(widget);
    }
    setCurrentWidget(widget);

    if (dynamic_cast<QLabel*>(widget) == NULL)
        setFrameStyle(QFrame::Panel | QFrame::Sunken);
    else
        setFrameStyle(QFrame::NoFrame);

    if (hasChanged) {
        KBibTeX::GUI::BibTeXEditor *newEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(widget);
        emit documentSwitch(oldEditor, newEditor);
    }
}

void MDIWidget::closeFile(OpenFileInfo *openFileInfo)
{
    QWidget *widget = openFileInfo->part(this)->widget();
    if (indexOf(widget) >= 0) {
        KBibTeX::GUI::BibTeXEditor *oldEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(currentWidget());
        removeWidget(widget);
        KBibTeX::GUI::BibTeXEditor *newEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(currentWidget());
        emit documentSwitch(oldEditor, newEditor);
    }
}

KBibTeX::GUI::BibTeXEditor *MDIWidget::editor()
{
    OpenFileInfo *ofi = OpenFileInfoManager::getOpenFileInfoManager()->currentFile();
    return dynamic_cast<KBibTeX::GUI::BibTeXEditor*>(ofi->part(this)->widget());
}
