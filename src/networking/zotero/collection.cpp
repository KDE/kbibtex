/***************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "internalnetworkaccessmanager.h"

using namespace Zotero;

const QString rootKey = QLatin1String("ROOT_KEY_KBIBTEX");
const int limit = 45;

class Zotero::Collection::Private
{
private:
    Zotero::Collection *p;

public:
    Private(Zotero::Collection *parent)
            : p(parent) {
        initialized = false;
    }

    KUrl baseUrl;
    bool initialized;

    QQueue<QString> downloadQueue;

    QHash<QString, QString> collectionToLabel;
    QHash<QString, QString> collectionToParent;
    QHash<QString, QVector<QString> > collectionToChildren;

    void requestZoteroUrl(const KUrl &url, const QString &parentId) {
        KUrl internalUrl = url;
        if (!url.queryItems().contains(QLatin1String("limit")))
            internalUrl.addQueryItem(QLatin1String("limit"), QString::number(limit));
        QNetworkRequest request(internalUrl);
        request.setRawHeader("Zotero-API-Version", "2");

        kDebug() << "Requesting from Zotero: " << internalUrl.pathOrUrl();

        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        connect(reply, SIGNAL(finished()), p, SLOT(finishedFetchingCollection()));
        reply->setProperty("parentId", QVariant::fromValue<QString>(parentId));
    }

    void runNextInDownloadQueue() {
        if (!downloadQueue.isEmpty()) {
            const QString head = downloadQueue.dequeue();
            KUrl url = baseUrl;
            url.addPath(QString(QLatin1String("/collections/%1/collections")).arg(head));
            requestZoteroUrl(url, head);
        } else {
            initialized = true;
            kDebug() << "Queue is empty, number of found collections:" << collectionToLabel.count();
            p->emitFinishedLoading();
        }
    }
};

const KUrl Collection::zoteroUrl = KUrl(QLatin1String("https://api.zotero.org"));

Collection *Collection::fromUserId(int userId, QObject *parent)
{
    KUrl baseUrl = zoteroUrl;
    baseUrl.setPath(QString(QLatin1String("/users/%1")).arg(userId));
    return new Collection(baseUrl, parent);
}

Collection *Collection::fromGroupId(int groupId, QObject *parent)
{
    KUrl baseUrl = zoteroUrl;
    baseUrl.setPath(QString(QLatin1String("/groups/%1")).arg(groupId));
    return new Collection(baseUrl, parent);
}

bool Collection::initialized() const
{
    return d->initialized;
}

QString Collection::collectionLabel(const QString &collectionId) const
{
    if (!d->initialized) return QString();

    if (collectionId == rootKey) /// root node
        return QLatin1String("Root node");

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

    if (collectionId == rootKey) /// root node
        return 0;

    return qHash(collectionId);
}

QString Collection::collectionFromNumericId(uint numericId) const
{
    if (numericId == 0) /// root node
        return rootKey;

    // TODO make those resolutions more efficient
    const QList<QString> keys = d->collectionToLabel.keys();
    foreach(const QString &key, keys) {
        if (numericId == qHash(key))
            return key;
    }
    return QString();
}

Collection::Collection(const KUrl &baseUrl, QObject *parent)
        : QObject(parent), d(new Zotero::Collection::Private(this))
{
    d->baseUrl = baseUrl;
    KUrl url = baseUrl;
    url.addPath(QLatin1String("/collections/top"));
    d->requestZoteroUrl(url, rootKey);
}

void Collection::finishedFetchingCollection()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    const QString parentId = reply->property("parentId").toString();

    if (reply->error() == QNetworkReply::NoError) {
        d->collectionToChildren[parentId] = QVector<QString>();

        QString nextPage;
        QXmlStreamReader xmlReader(reply);
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            const QXmlStreamReader::TokenType tt = xmlReader.readNext();
            if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("entry")) {
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
                    QVector<QString> vec =  d->collectionToChildren[parentId];
                    vec.append(key);
                    d->collectionToChildren[parentId] = vec;
                }
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QLatin1String("link")) {
                const QXmlStreamAttributes attrs = xmlReader.attributes();
                if (attrs.hasAttribute(QLatin1String("rel")) && attrs.hasAttribute(QLatin1String("href")) && attrs.value(QLatin1String("rel")) == QLatin1String("next"))
                    nextPage = attrs.value(QLatin1String("href")).toString();
            } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QLatin1String("feed"))
                break;
        }

        if (!nextPage.isEmpty()) {
            d->requestZoteroUrl(nextPage, parentId);
        } else
            d->runNextInDownloadQueue();
    }
}

void Collection::emitFinishedLoading()
{
    emit finishedLoading();
}
