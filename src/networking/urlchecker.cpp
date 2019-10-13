/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "urlchecker.h"

#include <QTimer>
#include <QSharedPointer>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QAtomicInteger>

#include <Entry>
#include <FileInfo>
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class UrlChecker::Private
{
private:
    UrlChecker *p;

public:
    QAtomicInteger<int> busyCounter;
    QSet<QUrl> urlsToCheck;

    Private(UrlChecker *parent)
            : p(parent)
    {
        /// nothing
    }

    void queueMoreOrFinish()
    {
        if (busyCounter.load() <= 0 && urlsToCheck.isEmpty()) {
            /// In case there are no running checks and the queue of URLs to check is empty,
            /// wait for a brief moment of time, then fire a 'finished' signal.
            QTimer::singleShot(100, p, [this]() {
                if (busyCounter.load() <= 0 && urlsToCheck.isEmpty())
                    QMetaObject::invokeMethod(p, "finished", Qt::DirectConnection, QGenericReturnArgument());
                else
                    /// It should not happen that when this timer is triggered the original condition is violated
                    qCCritical(LOG_KBIBTEX_NETWORKING) << "This cannot happen:" << busyCounter.load() << urlsToCheck.count();
            });
        } else {
            /// Initiate as many checks as possible
            while (!urlsToCheck.isEmpty() && busyCounter.load() <= 4)
                checkNextUrl();
        }
    }

    void checkNextUrl()
    {
        /// Immediately return if there are no URLs to check
        if (urlsToCheck.isEmpty()) return;
        /// Pop one URL from set of URLS to check
        auto firstUrlIt = urlsToCheck.begin();
        const QUrl url = *firstUrlIt;
        urlsToCheck.erase(firstUrlIt);

        QNetworkRequest request(url);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        busyCounter.ref();
        QObject::connect(reply, &QNetworkReply::finished, p, [this, reply]() {
            const QUrl url = reply->url();
            if (reply->error() != QNetworkReply::NoError) {
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::NetworkError), Q_ARG(QString, reply->errorString()));
                qCWarning(LOG_KBIBTEX_NETWORKING) << "NetworkError:" << reply->errorString() << url.toDisplayString();
            } else {
                const QByteArray data = reply->read(1024);
                if (data.isEmpty()) {
                    /// Instead of an 'emit' ...
                    QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UnknownError), Q_ARG(QString, QStringLiteral("No data received")));
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "UnknownError: No data received" << url.toDisplayString();
                } else {
                    const QString filename = url.fileName().toLower();
                    const bool filenameSuggestsHTML = filename.isEmpty() || filename.endsWith(QStringLiteral(".html")) || filename.endsWith(QStringLiteral(".htm"));
                    const bool filenameSuggestsPDF =  filename.endsWith(QStringLiteral(".pdf"));
                    const bool filenameSuggestsPostScript =  filename.endsWith(QStringLiteral(".ps"));
                    const bool containsHTML = data.contains("<!DOCTYPE HTML") || data.contains("<html") || data.contains("<HTML") || data.contains("<body") || data.contains("<body");
                    const bool containsPDF = data.startsWith("%PDF");
                    const bool containsPostScript = data.startsWith("%!");
                    if (filenameSuggestsPDF && containsPDF) {
                        /// Instead of an 'emit' ...
                        QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UrlValid), Q_ARG(QString, QString()));
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "UrlValid: Looks and smells like a PDF" << url.toDisplayString();
                    } else if (filenameSuggestsPostScript && containsPostScript) {
                        /// Instead of an 'emit' ...
                        QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UrlValid), Q_ARG(QString, QString()));
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "UrlValid: Looks and smells like a PostScript" << url.toDisplayString();
                    } else if (containsHTML) {
                        static const QRegularExpression error404(QStringLiteral("\\b404\\b"));
                        const QRegularExpressionMatch error404match = error404.match(data);
                        if (error404match.hasMatch()) {
                            /// Instead of an 'emit' ...
                            QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::Error404), Q_ARG(QString, QStringLiteral("Got error 404")));
                            qCWarning(LOG_KBIBTEX_NETWORKING) << "Error404" << url.toDisplayString();
                        } else if (filenameSuggestsHTML) {
                            /// Instead of an 'emit' ...
                            QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UrlValid), Q_ARG(QString, QString()));
                            qCWarning(LOG_KBIBTEX_NETWORKING) << "UrlValid: Looks and smells like a HTML" << url.toDisplayString();
                        } else {
                            /// Instead of an 'emit' ...
                            QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UnexpectedFileType), Q_ARG(QString, QStringLiteral("Filename's extension does not match content")));
                            qCWarning(LOG_KBIBTEX_NETWORKING) << "NotExpectedFileType (HTML): Filename's extension does not match content" << url.toDisplayString();
                        }
                    } else if (filenameSuggestsPDF != containsPDF) {
                        /// Instead of an 'emit' ...
                        QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UnexpectedFileType), Q_ARG(QString, QStringLiteral("Filename's extension does not match content")));
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "NotExpectedFileType (PDF): Filename's extension does not match content" << url.toDisplayString();
                    } else if (filenameSuggestsPostScript != containsPostScript) {
                        /// Instead of an 'emit' ...
                        QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UnexpectedFileType), Q_ARG(QString, QStringLiteral("Filename's extension does not match content")));
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "NotExpectedFileType (PostScript): Filename's extension does not match content" << url.toDisplayString();
                    } else {
                        /// Instead of an 'emit' ...
                        QMetaObject::invokeMethod(p, "urlChecked", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QUrl, url), Q_ARG(UrlChecker::Status, UrlChecker::UrlValid), Q_ARG(QString, QString()));
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "UrlValid: Cannot see any issued with this URL" << url.toDisplayString();
                    }
                }
            }
            busyCounter.deref();
            queueMoreOrFinish();
        });
    }
};

UrlChecker::UrlChecker(QObject *parent)
        : QObject(parent), d(new Private(this))
{
    /// nothing
}

UrlChecker::~UrlChecker()
{
    delete d;
}

void UrlChecker::startChecking(const File &bibtexFile)
{
    if (bibtexFile.count() < 1) {
        /// Nothing to do for empty bibliographies
        QTimer::singleShot(100, this, [this]() {
            emit finished();
        });
        return;
    }

    for (QSharedPointer<Element> element : bibtexFile) {
        /// Process only entries, not comments, preambles or macros
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        if (entry.isNull()) continue;

        /// Retrieve set of URLs per entry and add to set of URLS to be checked
        const QSet<QUrl> thisEntryUrls = FileInfo::entryUrls(entry, bibtexFile.property(File::Url).toUrl(), FileInfo::TestExistenceNo);
        for (const QUrl &u : thisEntryUrls)
            d->urlsToCheck.insert(u); ///< better?
    }

    if (d->urlsToCheck.isEmpty()) {
        /// No URLs identified in bibliography, so nothing to do
        QTimer::singleShot(100, this, [this]() {
            emit finished();
        });
        return;
    }

    d->queueMoreOrFinish();
}
