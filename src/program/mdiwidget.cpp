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
#include <kio/netaccess.h>
#include <kmimetypetrader.h>
#include <kparts/part.h>

#include "mdiwidget.h"

using namespace KBibTeX::Program;

class MDIWidget::MDIWidgetPrivate
{
public:
    struct OpenFileInfo {
        KUrl url;
        QString encoding;
        KParts::Part *part;
        QWidget *container;
    };

    MDIWidgetPrivate(MDIWidget *p)
            : p(p) {
        // nothing
    }

    QList<struct OpenFileInfo> openFiles;
    MDIWidget *p;
    bool getOpenFileInfo(const KUrl &url, struct OpenFileInfo &openFileInfo) {
        for (QList<struct OpenFileInfo>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it)
            if ((*it).url.equals(url)) {
                openFileInfo.url = (*it).url;
                openFileInfo.encoding = (*it).encoding;
                openFileInfo.part = (*it).part;
                openFileInfo.container = (*it).container;
                return true;
            }
        return false;
    }

    bool getOpenFileInfo(QWidget *container, struct OpenFileInfo &openFileInfo) {
        for (QList<struct OpenFileInfo>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it)
            if ((*it).container == container) {
                openFileInfo.url = (*it).url;
                openFileInfo.encoding = (*it).encoding;
                openFileInfo.part = (*it).part;
                openFileInfo.container = (*it).container;
                return true;
            }
        return false;
    }

    bool createPartAndWidget(struct OpenFileInfo &openFileInfo) {
        QString mimeTypeName = KIO::NetAccess::mimetype(openFileInfo.url, 0);
        kDebug() << "mimetype is " << mimeTypeName << endl;

        KService::List list = KMimeTypeTrader::self()->query(mimeTypeName, QString::fromLatin1("KParts/ReadWritePart"));
        KParts::ReadWritePart* part = NULL;
        for (KService::List::Iterator it = list.begin(); it != list.end(); ++it) {
            kDebug() << "service name is " << (*it)->name() << endl;
            if (part == NULL || (*it)->name().contains("BibTeX", Qt::CaseInsensitive))
                part = (*it)->createInstance<KParts::ReadWritePart>((QWidget*)p, (QObject*)p);
        }

        if (part == NULL) {
            kError() << "Cannot find part for mimetype " << mimeTypeName << endl;
            return false;
        }

        QWidget *newWidget = part->widget();
        newWidget->setParent(p);
        p->addWidget(newWidget);

        part->openUrl(openFileInfo.url);

        openFileInfo.part = part;
        openFileInfo.container = newWidget;

        return true;
    }

    bool addUniqueUrl(const KUrl &url, const QString &encoding) {
        for (QList<struct OpenFileInfo>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it)
            if ((*it).url.equals(url)) {
                (*it).encoding = encoding;
                p->setCurrentWidget((*it).container);
                return true;
            }

        qApp->setOverrideCursor(Qt::WaitCursor);

        struct OpenFileInfo openFileInfo;
        openFileInfo.url = url;
        openFileInfo.encoding = encoding;
        if (!createPartAndWidget(openFileInfo)) {
            qApp->restoreOverrideCursor();
            return false;
        }

        p->setCurrentWidget(openFileInfo.container);
        openFiles.append(openFileInfo);

        qApp->restoreOverrideCursor();
        return true;
    }

    bool removeUrl(const KUrl &url) {
        int i = 0;
        for (QList<struct OpenFileInfo>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it, ++i)
            if ((*it).url.equals(url)) {
                p->removeWidget((*it).container);
                openFiles.removeAt(i);
                return true;
            }

        return false;
    }
};

MDIWidget::MDIWidget(QWidget *parent)
        : QStackedWidget(parent), d(new MDIWidgetPrivate(this))
{
    QLabel *label = new QLabel("<qt>Welcome to <b>KBibTeX</b> for <b>KDE 4</b><br/><br/>Please select a file to open</qt>", this);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    addWidget(label);
}

bool MDIWidget::setUrl(const KUrl &url, const QString &encoding)
{
    struct MDIWidgetPrivate::OpenFileInfo oldOpenFileInfo;
    KUrl oldUrl = d->getOpenFileInfo(currentWidget(), oldOpenFileInfo) ? oldOpenFileInfo.url : KUrl();

    bool addingSucceeded = d->addUniqueUrl(url, encoding);

    if (!addingSucceeded)
        return false;

    struct MDIWidgetPrivate::OpenFileInfo newOpenFileInfo;
    if (!oldUrl.equals(url) && d->getOpenFileInfo(url, newOpenFileInfo)) {
        emit documentSwitch(editor(), oldUrl.isValid() ? dynamic_cast<KBibTeX::GUI::BibTeXEditor*>(oldOpenFileInfo.container) : NULL);
        emit activePartChanged(newOpenFileInfo.part);
    }

    return true;
}


bool MDIWidget::closeUrl(const KUrl &url)
{
    KBibTeX::GUI::BibTeXEditor *prevEditor = editor();

    bool wasClosing = d->removeUrl(url);

    if (wasClosing) emit documentSwitch(NULL, prevEditor);

    return true;
}

KUrl MDIWidget::currentUrl() const
{
    struct MDIWidgetPrivate::OpenFileInfo openFileInfo;
    if (d->getOpenFileInfo(currentWidget(), openFileInfo)) {
        return openFileInfo.url;
    }
    return KUrl();
}

KBibTeX::GUI::BibTeXEditor *MDIWidget::editor()
{
    struct MDIWidgetPrivate::OpenFileInfo openFileInfo;
    if (d->getOpenFileInfo(currentWidget(), openFileInfo)) {
        return dynamic_cast<KBibTeX::GUI::BibTeXEditor*>(openFileInfo.container);
    }

    return NULL;
}
