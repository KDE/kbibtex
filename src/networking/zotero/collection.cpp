/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "collection.h"

#include <QHash>
#include <QQueue>
#include <QVector>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QTimer>

#include <KLocalizedString>

#include "api.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

using namespace Zotero;

class Zotero::Collection::Private
{
private:
    Zotero::Collection *p;

public:
    QSharedPointer<Zotero::API> api;

    static const QString top;

    Private(QSharedPointer<Zotero::API> a, Zotero::Collection *parent)
            : p(parent), api(a) {
        initialized = false;
        busy = false;
    }

    bool initialized, busy;

    QQueue<QString> downloadQueue;

    QHash<QString, QString> collectionToLabel;
    QHash<QString, QString> collectionToParent;
    QHash<QString, QVector<QString> > collectionToChildren;

    QNetworkReply *requestZoteroUrl(const QUrl &url) {
        busy = true;
        QUrl internalUrl = url;
        api->addLimitToUrl(internalUrl);
        QNetworkRequest request = api->request(internalUrl);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        connect(reply, &QNetworkReply::finished, p, &Zotero::Collection::finishedFetchingCollection);
        return reply;
    }

    void runNextInDownloadQueue() {
        if (!downloadQueue.isEmpty()) {
            const QString head = downloadQueue.dequeue();
            QUrl url = api->baseUrl();
            url = url.adjusted(QUrl::StripTrailingSlash);
            url.setPath(url.path() + QString(QStringLiteral("/collections/%1/collections")).arg(head));
            if (api->inBackoffMode())
                /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
                QTimer::singleShot((api->backoffSecondsLeft() + 1) * 1000, p, [ = ]() {
                    requestZoteroUrl(url);
                });
            else
                requestZoteroUrl(url);
        } else {
            initialized = true;
            p->emitFinishedLoading();
        }
    }
};

const QString Zotero::Collection::Private::top = QStringLiteral("top");

Collection::Collection(QSharedPointer<Zotero::API> api, QObject *parent)
        : QObject(parent), d(new Zotero::Collection::Private(api, this))
{
    d->collectionToLabel[Private::top] = i18n("Library");

    QUrl url = api->baseUrl();
    url = url.adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path() + QStringLiteral("/collections/top"));
    if (api->inBackoffMode())
        /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
        QTimer::singleShot((api->backoffSecondsLeft() + 1) * 1000, this, [ = ]() {
        d->requestZoteroUrl(url);
    });
    else
        d->requestZoteroUrl(url);
}

Collection::~Collection()
{
    delete d;
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
    for (const QString &key : keys) {
        if (numericId == qHash(key))
            return key;
    }
    return QString();
}

void Collection::finishedFetchingCollection()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    QString parentId = Private::top;

    if (reply->hasRawHeader("Backoff")) {
        bool ok = false;
        int time = QString::fromLatin1(reply->rawHeader("Backoff").constData()).toInt(&ok);
        if (!ok) time = 10; ///< parsing argument of raw header 'Backoff' failed? 10 seconds is fallback
        d->api->startBackoff(time);
    } else if (reply->hasRawHeader("Retry-After")) {
        bool ok = false;
        int time = QString::fromLatin1(reply->rawHeader("Retry-After").constData()).toInt(&ok);
        if (!ok) time = 10; ///< parsing argument of raw header 'Retry-After' failed? 10 seconds is fallback
        d->api->startBackoff(time);
    }

    if (reply->error() == QNetworkReply::NoError) {
        QString nextPage;
        QXmlStreamReader xmlReader(reply);
        while (!xmlReader.atEnd() && !xmlReader.hasError()) {
            const QXmlStreamReader::TokenType tt = xmlReader.readNext();
            if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("title")) {
                /// Not perfect: guess author name from collection's title
                const QStringList titleFragments = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements).split(QStringLiteral(" / "));
                if (titleFragments.count() == 3)
                    d->collectionToLabel[Private::top] = i18n("%1's Library", titleFragments[1]);
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("entry")) {
                QString title, key;
                while (!xmlReader.atEnd() && !xmlReader.hasError()) {
                    const QXmlStreamReader::TokenType tt = xmlReader.readNext();
                    if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("title"))
                        title = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("key"))
                        key = xmlReader.readElementText(QXmlStreamReader::IncludeChildElements);
                    else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QStringLiteral("entry"))
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
            } else if (tt == QXmlStreamReader::StartElement && xmlReader.name() == QStringLiteral("link")) {
                const QXmlStreamAttributes attrs = xmlReader.attributes();
                if (attrs.hasAttribute(QStringLiteral("rel")) && attrs.hasAttribute(QStringLiteral("href")) && attrs.value(QStringLiteral("rel")) == QStringLiteral("next"))
                    nextPage = attrs.value(QStringLiteral("href")).toString();
                else if (attrs.hasAttribute(QStringLiteral("rel")) && attrs.hasAttribute(QStringLiteral("href")) && attrs.value(QStringLiteral("rel")) == QStringLiteral("self")) {
                    const QString text = attrs.value(QStringLiteral("href")).toString();
                    const int p1 = text.indexOf(QStringLiteral("/collections/"));
                    const int p2 = text.indexOf(QStringLiteral("/"), p1 + 14);
                    if (p1 > 0 && p2 > p1 + 14)
                        parentId = text.mid(p1 + 13, p2 - p1 - 13);
                }
            } else if (tt == QXmlStreamReader::EndElement && xmlReader.name() == QStringLiteral("feed"))
                break;
        }

        if (!nextPage.isEmpty()) {
            if (d->api->inBackoffMode())
                /// If Zotero asked to 'back off', wait until this period is over before issuing the next request
                QTimer::singleShot((d->api->backoffSecondsLeft() + 1) * 1000, this, [ = ]() {
                    d->requestZoteroUrl(nextPage);
                });
            else
                d->requestZoteroUrl(nextPage);
        } else
            d->runNextInDownloadQueue();
    } else {
        qCWarning(LOG_KBIBTEX_NETWORKING) << reply->errorString(); ///< something went wrong
        d->initialized = false;
        emitFinishedLoading();
    }
}

void Collection::emitFinishedLoading()
{
    d->busy = false;
    emit finishedLoading();
}
