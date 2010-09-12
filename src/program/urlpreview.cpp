/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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
#include <QLayout>
#include <QLabel>
#include <QMap>

#include <KUrl>
#include <KLocale>
#include <KComboBox>
#include <KIcon>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KService>
#include <kparts/part.h>
#include <KDebug>
#include <kio/netaccess.h>
#include <KPushButton>
#include <KRun>

#include <element.h>
#include <entry.h>
#include <file.h>
#include <fileinfo.h>
#include "urlpreview.h"

class UrlPreview::UrlPreviewPrivate
{
private:
    UrlPreview *p;
    QList<KUrl> urlList;
    KComboBox *urlComboBox;
    KPushButton *externalViewerButton;
    QLabel *message;
    QWidget *partWidget, *currentWidget;
    QMap<QString, KUrl> cbxEntryToUrl;
    QMap<KUrl, QWidget*> urlToWidget;
    QGridLayout *layout;

public:
    const Entry* entry;

    UrlPreviewPrivate(UrlPreview *parent)
            : p(parent), partWidget(NULL), currentWidget(NULL) {
        setupGUI();
    }

    void setupGUI() {
        layout = new QGridLayout(p);
        layout->setMargin(0);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);
        layout->setRowStretch(0, 0);
        layout->setRowStretch(1, 1);

        urlComboBox = new KComboBox(false, p);
        layout->addWidget(urlComboBox, 0, 0, 1, 1);

        externalViewerButton = new KPushButton(KIcon("document-open"), i18n("Open..."), p);
        layout->addWidget(externalViewerButton, 0, 1, 1, 1);

        message = new QLabel(i18n("No preview available"), p);
        layout->addWidget(message, 1, 0, 1, 2, Qt::AlignCenter);

        connect(externalViewerButton, SIGNAL(clicked()), p, SLOT(openExternally()));
        connect(urlComboBox, SIGNAL(activated(const QString &)), p, SLOT(urlSelected(const QString &)));
    }

    void update() {
        urlComboBox->clear();
        cbxEntryToUrl.clear();
        urlToWidget.clear();

        urlList = FileInfo::entryUrls(entry);
        for (QList<KUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
            QString fn = (*it).fileName();
            QString full = (*it).prettyUrl();
            QString text = fn.isEmpty() ? full : QString("%1 [%2]").arg(fn).arg(full.left(full.size() - fn.size()));
            urlComboBox->addItem(KIcon(KMimeType::iconNameForUrl(*it)), text);
            cbxEntryToUrl.insert(text, *it);
        }
        if (urlComboBox->count() > 0) {
            urlComboBox->setCurrentIndex(0);
            urlSelected(urlComboBox->currentText());
        }
        urlComboBox->setEnabled(urlComboBox->count() > 0);
        externalViewerButton->setEnabled(urlComboBox->count() > 0);
    }

    void urlSelected(const QString &text) {
        KUrl url = cbxEntryToUrl[text];
        QWidget *widget = urlToWidget[url];

        if (widget == NULL) {
            KParts::ReadOnlyPart* part = NULL;
            QString mimeType = KIO::NetAccess::mimetype(url, p);

            KService::Ptr serivcePtr = KMimeTypeTrader::self()->preferredService(mimeType, "KParts/ReadOnlyPart");
            if (!serivcePtr.isNull())
                part = serivcePtr->createInstance<KParts::ReadOnlyPart>((QWidget*)p, (QObject*)p);

            if (part != NULL) {
                urlToWidget.insert(url, part->widget());
                part->openUrl(url);
            } else
                urlToWidget.insert(url, message);
            widget = urlToWidget[url];
        }

        if (currentWidget != NULL)
            currentWidget->hide();

        widget->show();
        layout->addWidget(widget, 1, 0, 1, 2);
        currentWidget = widget;
    }

    void openExternally() {
        KUrl url(cbxEntryToUrl[urlComboBox->currentText()]);
        KRun::runUrl(url, KIO::NetAccess::mimetype(url, p), p);
    }

};

UrlPreview::UrlPreview(QWidget *parent)
        : QWidget(parent), d(new UrlPreviewPrivate(this))
{
    setEnabled(false);
}

void UrlPreview::setElement(Element* element, const File *)
{
    d->entry = dynamic_cast<const Entry*>(element);
    d->update();
}

void UrlPreview::urlSelected(const QString &text)
{
    d->urlSelected(text);
}

void UrlPreview::openExternally()
{
    d->openExternally();
}
