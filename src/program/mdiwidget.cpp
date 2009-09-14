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
#include <KMessageBox>
#include <kio/netaccess.h>
#include <kmimetypetrader.h>
#include <kparts/part.h>

#include "mdiwidget.h"

using namespace KBibTeX::Program;

class MDIWidget::MDIWidgetPrivate
{
public:
    struct Triple {
        KUrl url;
        QString encoding;
        QWidget *container;
    };

    MDIWidgetPrivate(MDIWidget *p)
            : p(p) {
        // nothing
    }

    QList<struct Triple> openFiles;
    MDIWidget *p;

    bool getTriple(const KUrl &url, struct Triple &triple) {
        for (QList<struct Triple>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it)
            if ((*it).url.equals(url)) {
                triple.url = (*it).url;
                triple.encoding = (*it).encoding;
                triple.container = (*it).container;
                return true;
            }
        return false;
    }

    bool getTriple(QWidget *container, struct Triple &triple) {
        for (QList<struct Triple>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it)
            if ((*it).container == container) {
                triple.url = (*it).url;
                triple.encoding = (*it).encoding;
                triple.container = (*it).container;
                return true;
            }
        return false;
    }

    QWidget *createWidget(const KUrl &url, const QString &encoding) {
        QString mimeTypeName = KIO::NetAccess::mimetype(url, 0);
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
            KMessageBox::error(p, QString("<qt>There is no editor component available for mime type\n<b>%1</b>\nto open file\n%2</qt>").arg(mimeTypeName).arg(url.fileName()), "Error opening file");
            return NULL;
        }

        QWidget *newWidget = part->widget();
        newWidget->setParent(p);
        p->addWidget(newWidget);

        part->openUrl(url);

        return newWidget;
    }

    bool addUniqueUrl(const KUrl &url, const QString &encoding) {
        for (QList<struct Triple>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it)
            if ((*it).url.equals(url)) {
                (*it).encoding = encoding;
                p->setCurrentWidget((*it).container);
                return true;
            }

        qApp->setOverrideCursor(Qt::WaitCursor);

        struct Triple triple;
        triple.url = url;
        triple.encoding = encoding;
        triple.container = createWidget(url, encoding);

        if (triple.container == NULL) {
            qApp->restoreOverrideCursor();
            return false;
        }

        p->setCurrentWidget(triple.container);
        openFiles.append(triple);

        qApp->restoreOverrideCursor();
        return true;
    }

    bool removeUrl(const KUrl &url) {
        int i = 0;
        for (QList<struct Triple>::Iterator it = openFiles.begin(); it != openFiles.end(); ++it, ++i)
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
    struct MDIWidgetPrivate::Triple triple;
    KUrl oldUrl = d->getTriple(currentWidget(), triple) ? triple.url : KUrl();

    bool addingSucceeded = d->addUniqueUrl(url, encoding);

    if (!addingSucceeded)
        return false;

    if (!oldUrl.equals(url)) emit documentSwitch();

    return true;
}


bool MDIWidget::closeUrl(const KUrl &url)
{
    bool wasClosing = d->removeUrl(url);

    if (wasClosing) emit documentSwitch();

    return true;
}

KUrl MDIWidget::currentUrl() const
{
    struct MDIWidgetPrivate::Triple triple;
    if (d->getTriple(currentWidget(), triple)) {
        return triple.url;
    }
    return KUrl();
}

KBibTeX::GUI::BibTeXEditorInterface *MDIWidget::editor()
{
    struct MDIWidgetPrivate::Triple triple;
    if (d->getTriple(currentWidget(), triple)) {
        return dynamic_cast<KBibTeX::GUI::BibTeXEditorInterface*>(triple.container);
    }

    return NULL;
}
