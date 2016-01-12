/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "collection.h"

#include <QHash>
#include <QQueue>
#include <QVector>
#include <QNetworkReply>
#include <QXmlStreamReader>

#include <KDebug>
#include <KLocale>

#include "api.h"

#include "internalnetworkaccessmanager.h"

using namespace Zotero;

class Zotero::Collection::Private
{
private:
    Zotero::Collection *p;
    Zotero::API *api;

public:
    static const QString top;

    Private(API *a, Zotero::Collection *parent)
            : p(parent), api(a) {
        initialized = false;
        busy = false;
    }

    bool initialized, busy;

    QQueue<QString> downloadQueue;

    QHash<QString, QString> collectionToLabel;
    QHash<QString, QString> collectionToParent;
    QHash<QString, QVector<QString> > collectionToChildren;

    QNetworkReply *requestZoteroUrl(const KUrl &url) {
        busy = true;
        KUrl internalUrl = url;
        api->addLimitToUrl(internalUrl);
        QNetworkRequest request = api->request(internalUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        connect(reply, SIGNAL(finished()), p, SLOT(finishedFetchingCollection()));
        return reply;
    }

    void runNextInDownloadQueue() {
        if (!downloadQueue.isEmpty()) {
            const QString head = downloadQueue.dequeue();
            KUrl url = api->baseUrl();
            url.addPath(QString(QLatin1String("/collections/%1/collections")).arg(head));
            requestZoteroUrl(url);
        } else {
            initialized = true;
            kDebug() << "Queue is empty, number of found collections:" << collectionToLabel.count();
            p->emitFinishedLoading();
        }
    }
};

const QString Zotero::Collection::Private::top = QLatin1String("top");

Collection::Collection(API *api, QObject *parent)
        : QObject(parent), d(new Zotero::Collection::Private(api, this))
{
    d->collectionToLabel[Private::top] = i18n("Library");

    KUrl url = api->baseUrl();
    url.addPath(QLatin1String("/collections/top"));
    d->requestZoteroUrl(url);
}

bool Collection::initialized() const
{
    return d->initialized;
}

bool Collection::busy() const
{
    return d->busy;
}

QString Collection::collectionLabel(const QString &collectionId) const
{
    if (!d->initialized) return QString();

    return d->collectionToLabel[collectionId];
}

QString Collection::collectionParent(const QString &collectionId) const
{
    if (!d->initialized) return QString();

    return d->collectionToParent[collectionId];
}

QVector<QString> Collection::collectionChildren(const QString &collectionId) const
{
    if (!d->initialized) return QVector<QString>();

    return QVector<QString>(d->collectionToChildren[collectionId]);
}

uint Collection::collectionNumericId(const QString &collectionId) const
{
    if (!d->initialized) return 0;

    if (collectionId == Private::top) /// root node
        return 0;

    return qHash(collectionId);
}

QString Collection::collectionFromNumericId(uint numericId) const
{
    if (numericId == 0) /// root node
        return Private::top;

    // TODO make those resolutions more efficient
    const QList<QString> keys = d->collectionToLabel.keys();
    foreach(const QString &key, keys) {
        if (numericId == qHash(key))
            return key;
    }
    return QString();
}

void Collection::finishedFetchingCollection()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    QString parentId = Private::top;

    if (reply->error() == QNetworkReply::NoError) {
        QString nextPage;
        QXmlStreamReader xmlReader(reply);
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            const QXmlStreamReader::TokenType tt = xmlReader.readNext();
            if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("title")) {
                /// Not perfect: guess author name from collection's title
                const QStringList titleFragments = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements).split(QLatin1String(" / "));
                if (titleFragments.count() == 3)
                    d->collectionToLabel[Private::top] = i18n("%1's Library", titleFragments[1]);
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("entry")) {
                QString title, key;
                while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                    const QXmlStreamReader::TokenType tt = xmlReader.readNext();
                    if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("title"))
                        title = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("key"))
                        key = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("entry"))
                        break;
                }

                if (!key.isEmpty() && !title.isEmpty()) {
                    d->downloadQueue.enqueue(key);
                    d->collectionToLabel.insert(key, title);
                    d->collectionToParent.insert(key, parentId);
                    QVector<QString> vec = d->collectionToChildren[parentId];
                    vec.append(key);
                    d->collectionToChildren[parentId] = vec;
                }
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("link")) {
                const QXmlStreamAttributes attrs = xmlReader.attributes();
                if (attrs.hasAttribute(QLatin1String("rel")) && attrs.hasAttribute(QLatin1String("href")) && attrs.value(QLatin1String("rel")) == QLatin1String("next"))
                    nextPage = attrs.value(QLatin1String("href")).toString();
                else if (attrs.hasAttribute(QLatin1String("rel")) && attrs.hasAttribute(QLatin1String("href")) && attrs.value(QLatin1String("rel")) == QLatin1String("self")) {
                    const QString text = attrs.value(QLatin1String("href")).toString();
                    const int p1 = text.indexOf(QLatin1String("/collections/"));
                    const int p2 = text.indexOf(QLatin1String("/"), p1 + 14);
                    if (p1 > 0 && p2 > p1 + 14)
                        parentId = text.mid(p1 + 13, p2 - p1 - 13);
                }
            } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("feed"))
                break;
        }

        if (!nextPage.isEmpty()) {
            d->requestZoteroUrl(nextPage);
        } else
            d->runNextInDownloadQueue();
    } else {
        kWarning() << reply->errorString(); ///< something went wrong
        d->initialized = false;
        emitFinishedLoading();
    }
}

void Collection::emitFinishedLoading()
{
    d->busy = false;
    emit finishedLoading();
}
