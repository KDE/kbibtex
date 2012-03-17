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

#include <QList>
#include <QPair>
#include <QLabel>
#include <QApplication>
#include <QSignalMapper>
#include <QLayout>

#include <KDebug>
#include <KPushButton>
#include <KIcon>
#include <KMessageBox>
#include <kmimetypetrader.h>
#include <kparts/part.h>
#include <kio/netaccess.h>

#include "mdiwidget.h"
#include "openfileinfo.h"

class MDIWidget::MDIWidgetPrivate
{
private:
    void createWelcomeWidget() {
        welcomeWidget = new QWidget(p);
        QGridLayout *layout = new QGridLayout(welcomeWidget);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);
        layout->setRowStretch(2, 0);
        layout->setRowStretch(3, 1);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);
        layout->setColumnStretch(2, 0);
        layout->setColumnStretch(3, 1);

        QLabel *label = new QLabel(i18n("<qt>Welcome to <b>KBibTeX</b> for <b>KDE 4</b></qt>"), p);
        layout->addWidget(label, 1, 1, 1, 2, Qt::AlignHCenter | Qt::AlignTop);

        KPushButton *buttonNew = new KPushButton(KIcon("document-new"), i18n("New"), p);
        layout->addWidget(buttonNew, 2, 1, 1, 1, Qt::AlignLeft | Qt::AlignBottom);
        connect(buttonNew, SIGNAL(clicked()), p, SIGNAL(documentNew()));

        KPushButton *buttonOpen = new KPushButton(KIcon("document-open"), i18n("Open ..."), p);
        layout->addWidget(buttonOpen, 2, 2, 1, 1, Qt::AlignRight | Qt::AlignBottom);
        connect(buttonOpen, SIGNAL(clicked()), p, SIGNAL(documentOpen()));

        p->addWidget(welcomeWidget);
    }

public:
    MDIWidget *p;
    OpenFileInfo *currentFile;
    QWidget *welcomeWidget;
    QSignalMapper signalMapperCompleted;

    MDIWidgetPrivate(MDIWidget *parent)
            : p(parent), currentFile(NULL) {
        createWelcomeWidget();

        connect(&signalMapperCompleted, SIGNAL(mapped(QObject*)), p, SLOT(slotCompleted(QObject*)));
    }

    void addToMapper(OpenFileInfo *openFileInfo) {
        KParts::ReadOnlyPart *part = openFileInfo->part(p);
        signalMapperCompleted.setMapping(part, openFileInfo);
        connect(part, SIGNAL(completed()), &signalMapperCompleted, SLOT(map()));
    }
};

MDIWidget::MDIWidget(QWidget *parent)
        : QStackedWidget(parent), d(new MDIWidgetPrivate(this))
{
    // nothing
}

void MDIWidget::setFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr)
{
    BibTeXEditor *oldEditor = NULL;
    bool hasChanged = true;

    KParts::Part* part = openFileInfo == NULL ? NULL : openFileInfo->part(this, servicePtr);
    QWidget *widget = d->welcomeWidget;
    if (part != NULL) {
        widget = part->widget();
        //widget->setParent(this); // FIXME: necessary?
    } else if (openFileInfo != NULL) {
        KMessageBox::error(this, i18n("No part available for file '%1'.", openFileInfo->url().fileName()), i18n("No part available"));
        OpenFileInfoManager::getOpenFileInfoManager()->close(openFileInfo);
        return;
    }

    if (indexOf(widget) >= 0) {
        oldEditor = dynamic_cast<BibTeXEditor *>(currentWidget());
        hasChanged = widget != currentWidget();
    } else {
        addWidget(widget);
        d->addToMapper(openFileInfo);
    }
    setCurrentWidget(widget);
    d->currentFile = openFileInfo;

    if (hasChanged) {
        BibTeXEditor *newEditor = dynamic_cast<BibTeXEditor *>(widget);
        emit activePartChanged(part);
        emit documentSwitch(oldEditor, newEditor);
    }

    if (openFileInfo != NULL) {
        KUrl url = openFileInfo->url();
        if (url.isValid())
            emit setCaption(QString("%1 [%2]").arg(openFileInfo->shortCaption()).arg(openFileInfo->fullCaption()));
        else
            emit setCaption(openFileInfo->shortCaption());
    } else
        emit setCaption("");
}

BibTeXEditor *MDIWidget::editor()
{
    OpenFileInfo *ofi = OpenFileInfoManager::getOpenFileInfoManager()->currentFile();
    return dynamic_cast<BibTeXEditor*>(ofi->part(this)->widget());
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
        kDebug() << "Url changed from " << oldUrl.pathOrUrl() << " to " << newUrl.pathOrUrl() << endl;
        OpenFileInfoManager::getOpenFileInfoManager()->changeUrl(ofi, newUrl);

        /// completely opened or saved files should be marked as "recently used"
        ofi->addFlags(OpenFileInfo::RecentlyUsed);

        emit setCaption(QString("%1 [%2]").arg(ofi->shortCaption()).arg(ofi->fullCaption()));
    }
}
