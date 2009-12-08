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
    OpenFileInfo *currentFile;
    QLabel *welcomeLabel;

    MDIWidgetPrivate(MDIWidget *parent)
            : p(parent), currentFile(NULL) {
        welcomeLabel = new QLabel(i18n("<qt>Welcome to <b>KBibTeX</b> for <b>KDE 4</b><br/><br/>Please select a file to open</qt>"), p);
        welcomeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        p->addWidget(welcomeLabel);
    }
};

MDIWidget::MDIWidget(QWidget *parent)
        : QStackedWidget(parent), d(new MDIWidgetPrivate(this))
{
    // nothing
}

void MDIWidget::setFile(OpenFileInfo *openFileInfo)
{
    KBibTeX::GUI::BibTeXEditor *oldEditor = NULL;
    bool hasChanged = true;

    KParts::Part* part = openFileInfo->part(this);
    QWidget *widget = part->widget();
    widget->setParent(this);

    if (indexOf(widget) >= 0) {
        oldEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(currentWidget());
        hasChanged = widget != currentWidget();
    } else {
        addWidget(widget);
    }
    setCurrentWidget(widget);
    d->currentFile = openFileInfo;

    if (dynamic_cast<QLabel*>(widget) == NULL)
        setFrameStyle(QFrame::Panel | QFrame::Sunken);
    else
        setFrameStyle(QFrame::NoFrame);

    if (hasChanged) {
        KBibTeX::GUI::BibTeXEditor *newEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(widget);
        emit activePartChanged(part);
        emit documentSwitch(oldEditor, newEditor);
    }
}

void MDIWidget::closeFile(OpenFileInfo *openFileInfo)
{
    kDebug() << "closeFile" << endl;

    QWidget *widget = openFileInfo->part(this)->widget();
    if (indexOf(widget) >= 0) {
        QWidget *curWidget = currentWidget();

        if (curWidget == widget) {
            KBibTeX::GUI::BibTeXEditor *oldEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(widget);
            setCurrentWidget(d->welcomeLabel);
            emit activePartChanged(NULL);
            emit documentSwitch(oldEditor, NULL);
        }

        removeWidget(widget);
    }
}

KBibTeX::GUI::BibTeXEditor *MDIWidget::editor()
{
    OpenFileInfo *ofi = OpenFileInfoManager::getOpenFileInfoManager()->currentFile();
    return dynamic_cast<KBibTeX::GUI::BibTeXEditor*>(ofi->part(this)->widget());
}

OpenFileInfo *MDIWidget::currentFile()
{
    return d->currentFile;
}
