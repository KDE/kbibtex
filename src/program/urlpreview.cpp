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

#include <typeinfo>

#include <QList>
#include <QLayout>
#include <QLabel>
#include <QMap>
#include <QFileInfo>
#include <QDesktopServices>
#include <QCheckBox>
#include <QStackedWidget>

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
#include <KDebug>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KTemporaryFile>

#include <element.h>
#include <entry.h>
#include <file.h>
#include <fileinfo.h>
#include "urlpreview.h"

class UrlPreview::UrlPreviewPrivate
{
private:
    UrlPreview *p;
    KComboBox *urlComboBox;
    KPushButton *externalViewerButton;
    QCheckBox *onlyLocalFilesCheckBox;
    QStackedWidget *stackedWidget;
    QLabel *message;
    QMap<int, KUrl> cbxEntryToUrl;

public:
    const Entry* entry;
    KUrl baseUrl;

    UrlPreviewPrivate(UrlPreview *parent)
            : p(parent) {
        setupGUI();
    }

    void setupGUI() {
        QVBoxLayout *layout = new QVBoxLayout(p);
        layout->setMargin(0);

        QHBoxLayout *innerLayout = new QHBoxLayout();
        layout->addLayout(innerLayout, 0);
        urlComboBox = new KComboBox(false, p);
        innerLayout->addWidget(urlComboBox, 1);

        externalViewerButton = new KPushButton(KIcon("document-open"), i18n("Open..."), p);
        innerLayout->addWidget(externalViewerButton, 0);

        onlyLocalFilesCheckBox = new QCheckBox(i18n("Only local files"), p);
        layout->addWidget(onlyLocalFilesCheckBox, 0);

        stackedWidget = new QStackedWidget(p);
        layout->addWidget(stackedWidget, 1);
        stackedWidget->hide();

        message = new QLabel(i18n("No preview available"), p);
        message->setAlignment(Qt::AlignCenter);
        layout->addWidget(message, 1);

        connect(externalViewerButton, SIGNAL(clicked()), p, SLOT(openExternally()));
        connect(urlComboBox, SIGNAL(activated(int)), stackedWidget, SLOT(setCurrentIndex(int)));
        connect(onlyLocalFilesCheckBox, SIGNAL(toggled(bool)), p, SLOT(onlyLocalFilesChanged()));
    }

    void update() {
        urlComboBox->clear();
        while (stackedWidget->count() > 0)
            stackedWidget->removeWidget(stackedWidget->currentWidget());

        int localUrlIndex = 0; /// if no url below is local, use first entry as fall-back
        QList<KUrl> urlList = FileInfo::entryUrls(entry, baseUrl);
        for (QList<KUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
            if (onlyLocalFilesCheckBox->isChecked() && !(*it).isLocalFile()) continue;

            QString fn = (*it).fileName();
            QString full = (*it).pathOrUrl();
            QString dir = full.left(full.size() - fn.size());
            QString text = fn.isEmpty() ? full : (dir.isEmpty() ? fn : QString("%1 [%2]").arg(fn).arg(dir));
            QPair<QString, KIcon> mimeTypeIcon = mimeType(*it);
            urlComboBox->addItem(mimeTypeIcon.second, text);
            if ((*it).isLocalFile()) /// memorize url's index in drop-down list
                localUrlIndex = urlComboBox->count() - 1;
            cbxEntryToUrl.insert(urlComboBox->count() - 1, *it);

            KParts::ReadOnlyPart* part = NULL;
            KService::Ptr serivcePtr = KMimeTypeTrader::self()->preferredService(mimeTypeIcon.first, "KParts/ReadOnlyPart");
            if (!serivcePtr.isNull())
                part = serivcePtr->createInstance<KParts::ReadOnlyPart>((QWidget*)p, (QObject*)p);
            if (part != NULL) {
                stackedWidget->addWidget(part->widget());
                part->openUrl(*it);
            } else {
                QLabel *label = new QLabel(i18n("Cannot create preview for\n%1", (*it).pathOrUrl()), stackedWidget);
                message->setAlignment(Qt::AlignCenter);
                stackedWidget->addWidget(label);
            }
        }

        urlComboBox->setEnabled(urlComboBox->count() > 0);
        externalViewerButton->setEnabled(urlComboBox->count() > 0);

        if (urlComboBox->count() > 0) {
            urlComboBox->setCurrentIndex(localUrlIndex);
            message->hide();
            stackedWidget->show();
        } else {
            message->setText(i18n("No preview available"));
            message->show();
            stackedWidget->hide();
        }
    }

    void openExternally() {
        KUrl url(cbxEntryToUrl[urlComboBox->currentIndex()]);
        QDesktopServices::openUrl(url); // TODO KDE way?
    }

    QPair<QString, KIcon> mimeType(const KUrl &url) {
        if (!url.isLocalFile() && url.fileName().isEmpty()) {
            /// URLs not pointing to a specific file should be opened with a web browser component
            kDebug() << "Falling back to text/html";
            return QPair<QString, KIcon>(QLatin1String("text/html"), KIcon("text-html"));
        }

        int accuracy = 0;
        KMimeType::Ptr mimeTypePtr = KMimeType::findByUrl(url, 0, url.isLocalFile(), true, &accuracy);
        if (accuracy < 50) {
            kDebug() << "discarding mime type " << mimeTypePtr->name() << ", trying filename ";
            mimeTypePtr = KMimeType::findByPath(url.fileName(), 0, true, &accuracy);
        }
        QString mimeTypeName = mimeTypePtr->name();

        if (mimeTypeName == QLatin1String("application/octet-stream")) {
            /// application/octet-stream is a fall-back if KDE did not know better
            kDebug() << "Falling back to text/html";
            return QPair<QString, KIcon>(QLatin1String("text/html"), KIcon("text-html"));
        }

        KIcon icon = KIcon(mimeTypePtr->iconName());
        kDebug() << "For url " << url.pathOrUrl() << " selected mime type " << mimeTypeName << " (icon name " << mimeTypePtr->iconName() << ")";

        return QPair<QString, KIcon>(mimeTypeName, icon);
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

void UrlPreview::openExternally()
{
    d->openExternally();
}

void UrlPreview::setBibTeXUrl(const KUrl&url)
{
    d->baseUrl = url;
}

void UrlPreview::onlyLocalFilesChanged()
{
    d->update();
}
