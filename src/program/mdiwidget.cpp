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
#include <QSignalMapper>

#include <KDebug>
#include <KMessageBox>
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
    QSignalMapper signalMapperCompleted;

    MDIWidgetPrivate(MDIWidget *parent)
            : p(parent), currentFile(NULL) {
        welcomeLabel = new QLabel(i18n("<qt>Welcome to <b>KBibTeX</b> for <b>KDE 4</b><br/><br/>Please select a file to open</qt>"), p);
        welcomeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
        p->addWidget(welcomeLabel);

        connect(&signalMapperCompleted, SIGNAL(mapped(QObject*)), p, SLOT(slotCompleted(QObject*)));
    }

    void addToMapper(OpenFileInfo *openFileInfo) {
        KParts::ReadWritePart *part = openFileInfo->part(p);
        signalMapperCompleted.setMapping(part, openFileInfo);
        connect(part, SIGNAL(completed()), &signalMapperCompleted, SLOT(map()));
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

    KParts::Part* part = openFileInfo == NULL ? NULL : openFileInfo->part(this);
    QWidget *widget = d->welcomeLabel;
    if (part != NULL) {
        widget = part->widget();
        widget->setParent(this);
    } else if (openFileInfo != NULL) {
        KMessageBox::error(this, i18n("No part available for file '%1'.", openFileInfo->url().fileName()), i18n("No part available"));
        OpenFileInfoManager::getOpenFileInfoManager()->close(openFileInfo);
        return;
    }

    if (indexOf(widget) >= 0) {
        oldEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(currentWidget());
        hasChanged = widget != currentWidget();
    } else {
        addWidget(widget);
        d->addToMapper(openFileInfo);
    }
    setCurrentWidget(widget);
    d->currentFile = openFileInfo;

    if (hasChanged) {
        KBibTeX::GUI::BibTeXEditor *newEditor = dynamic_cast<KBibTeX::GUI::BibTeXEditor *>(widget);
        emit activePartChanged(part);
        emit documentSwitch(oldEditor, newEditor);
    }
}

void MDIWidget::closeFile(OpenFileInfo *openFileInfo)
{
    kDebug() << "closeFile" << endl;

    QWidget *widget = openFileInfo->part(this) != NULL ? openFileInfo->part(this)->widget() : NULL;
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

void MDIWidget::slotCompleted(QObject *obj)
{
    OpenFileInfo *ofi = static_cast<OpenFileInfo*>(obj);
    KUrl oldUrl = ofi->url();
    KUrl newUrl = ofi->part(this)->url();

    if (!oldUrl.equals(newUrl)) {
        kDebug() << "Url changed from " << oldUrl.prettyUrl() << " to " << newUrl.prettyUrl() << endl;
        OpenFileInfoManager::getOpenFileInfoManager()->changeUrl(ofi, newUrl);
    }
}
