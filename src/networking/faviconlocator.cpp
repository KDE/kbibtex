/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "faviconlocator.h"

#include <QStack>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>

#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

static int earliest(const QString &haystack, const QSet<QString> &needles, const int haystackFrom = 0) {
    int result = INT_MAX;
    for (const QString &needle : needles) {
        const int p = haystack.indexOf(needle, haystackFrom);
        if (p >= 0 && p < result)
            result = p;
    }
    return result == INT_MAX ? -1 : result;
}

class FavIconLocator::Private
{
private:
    static const QRegularExpression invalidChars;
    static const QString cacheDirectory;

    enum class UrlType { Cache, FavIcon, Website };
    struct TypedUrl {
        UrlType urlType;
        QUrl url;
    };
    QStack<TypedUrl> typedUrlStack;

    FavIconLocator *parent;

    const QString fileNameStem;
    QString originalUrls;

public:
    QIcon favIcon;

    Private(FavIconLocator *_parent, const QUrl &webpageUrl, const QUrl &suggestedFavIconUrl)
            : parent(_parent), fileNameStem(cacheDirectory + webpageUrl.toDisplayString().remove(QStringLiteral("https://")).remove(QStringLiteral("http://")).remove(invalidChars)), favIcon(QIcon::fromTheme(QStringLiteral("applications-internet")))
    {
        QDir().mkpath(cacheDirectory);

        if (webpageUrl.isValid()) {
            QUrl defaultFavIconUrl{webpageUrl};
            defaultFavIconUrl.setPath(QStringLiteral("/favicon.ico"));
            typedUrlStack.push({UrlType::FavIcon, defaultFavIconUrl});

            typedUrlStack.push({UrlType::Website, webpageUrl});
            originalUrls = InternalNetworkAccessManager::removeApiKey(webpageUrl).toDisplayString();
        }
        if (suggestedFavIconUrl.isValid()) {
            typedUrlStack.push({UrlType::FavIcon, suggestedFavIconUrl});
            if (!originalUrls.isEmpty())
                originalUrls.append(QStringLiteral(" and "));
            originalUrls.append(InternalNetworkAccessManager::removeApiKey(suggestedFavIconUrl).toDisplayString());
        }
        typedUrlStack.push({UrlType::Cache, QUrl()});

        processNextInStack();
    }

    void signalIcon(const QIcon &favIcon) {
        this->favIcon = favIcon;
        QTimer::singleShot(100, parent, [this]() {
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
            QMetaObject::invokeMethod(parent, "gotIcon", Qt::QueuedConnection, QGenericReturnArgument(), Q_ARG(QIcon, this->favIcon));
#else // QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
            QMetaObject::invokeMethod(parent, "gotIcon", Qt::QueuedConnection, Q_ARG(QIcon, this->favIcon));
#endif
        });
    }

    void processNextInStack()
    {
        if (typedUrlStack.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "All methods to locate FavIcon exhausted, giving up for original URL(s)" << originalUrls;
            return;
        }

        TypedUrl cur{typedUrlStack.pop()};

        if (cur.urlType == UrlType::Cache) {
            // Try to load FavIcon from cache and if successful, signal the FavIcon to anyone who is interested.
            static const QStringList fileNameExtensions {QStringLiteral(".png"), QStringLiteral(".ico")};
            for (const QString &extension : fileNameExtensions) {
                const QString fileName = fileNameStem + extension;
                const QFileInfo fi(fileName);
                if (fi.exists(fileName)) {
                    if (fi.lastModified().daysTo(QDateTime::currentDateTime()) > 90) {
                        // If FavIcon is older than 90 days, delete it and fetch current one
                        QFile::remove(fileName);
                    } else {
                        qCDebug(LOG_KBIBTEX_NETWORKING) << "Found cached FavIcon for" << originalUrls << "in file" << fileName;
                        signalIcon(QIcon(fileName));
                        return;
                    }
                }
            }
            processNextInStack();
        } else if (cur.urlType == UrlType::FavIcon) {
            qCDebug(LOG_KBIBTEX_NETWORKING) << "Requesting FavIcon URL" << cur.url.toDisplayString() << "for original URLs" << originalUrls;
            QNetworkRequest request(cur.url);
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
            connect(reply, &QNetworkReply::finished, parent, [this, reply, cur]() {
                const QByteArray iconData = reply->readAll();
                if (iconData.size() > 10) {
                    QString extension;
                    if (iconData[1] == 'P' && iconData[2] == 'N' && iconData[3] == 'G') {
                        // PNG files have string "PNG" at second to fourth byte
                        extension = QStringLiteral(".png");
                    } else if (iconData[0] == static_cast<char>(0x00) && iconData[1] == static_cast<char>(0x00) && iconData[2] == static_cast<char>(0x01) && iconData[3] == static_cast<char>(0x00)) {
                        // Microsoft Icon have first two bytes always 0x0000,
                        // third and fourth byte is 0x0001 (for .ico)
                        extension = QStringLiteral(".ico");
                    } else if (iconData[0] == '<') {
                        // HTML or XML code
                        const QString htmlCode = QString::fromUtf8(iconData);
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Received XML or HTML data from " << InternalNetworkAccessManager::removeApiKey(cur.url).toDisplayString() << ": " << htmlCode.left(128);
                    } else {
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Favicon is of unknown format: " << InternalNetworkAccessManager::removeApiKey(cur.url).toDisplayString();
                    }

                    if (!extension.isEmpty()) {
                        const QString filename = fileNameStem + extension;

                        QFile iconFile(filename);
                        if (iconFile.open(QFile::WriteOnly)) {
                            iconFile.write(iconData);
                            iconFile.close();
                            qCDebug(LOG_KBIBTEX_NETWORKING) << "Got FavIcon from URL" << InternalNetworkAccessManager::removeApiKey(cur.url).toDisplayString() << "stored in" << filename;
                            signalIcon(QIcon(filename));
                            return;
                        } else {
                            qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not save FavIcon data from URL" << InternalNetworkAccessManager::removeApiKey(cur.url).toDisplayString() << "to file" << filename;
                        }
                    }
                } else {
                    // Unlikely that an FavIcon's data is less than 10 bytes, must be an error.
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Received invalid FavIcon data from " << InternalNetworkAccessManager::removeApiKey(cur.url).toDisplayString();
                }

                processNextInStack();
            });
        } else if (cur.urlType == UrlType::Website) {
            QNetworkRequest request(cur.url);
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
            QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
            connect(reply, &QNetworkReply::finished, parent, [this, reply, cur]() {
                QUrl favIconUrl;

                // Assume that favicon information is within the first 16K of HTML code
                const QString htmlCode = QString::fromUtf8(reply->readAll()).left(16384);
                // Some ugly but hopefully fast/flexible/robust HTML code parsing
                int p1 = -1;
                while (!favIconUrl.isValid() && (p1 = htmlCode.indexOf(QStringLiteral("<link "), p1 + 5)) > 0) {
                    const int p2 = htmlCode.indexOf(QLatin1Char('>'), p1 + 5);
                    if (p2 > p1) {
                        const int p3 = htmlCode.indexOf(QStringLiteral("rel=\""), p1 + 5);
                        if (p3 > p1 && p3 < p2) {
                            const int p4 = htmlCode.indexOf(QLatin1Char('"'), p3 + 5);
                            if (p4 > p3 && p4 < p2) {
                                const QString relValue = htmlCode.mid(p3 + 5, p4 - p3 - 5);
                                if (relValue == QStringLiteral("icon") || relValue == QStringLiteral("shortcut icon")) {
                                    const int p5 = earliest(htmlCode, {QStringLiteral("href=\""), QStringLiteral("href=")}, p1 + 5);
                                    if (p5 > p1 && p5 < p2) {
                                        const int p6 = earliest(htmlCode, {QStringLiteral("\""), QStringLiteral(" "), QStringLiteral(">")}, p5 + 6);
                                        if (p6 > p5 + 5 && p6 <= p2) {
                                            QString hrefValue = htmlCode.mid(p5 + 6, p6 - p5 - 6).replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('>'), QLatin1String("&gt;")).replace(QLatin1Char('<'), QLatin1String("&lt;"));
                                            // Do some resolving in case favicon URL in HTML code is relative
                                            favIconUrl = cur.url.resolved(QUrl(hrefValue));
                                            if (favIconUrl.isValid())
                                                break;
                                            else
                                                favIconUrl.clear();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (favIconUrl.isValid()) {
                    qCDebug(LOG_KBIBTEX_NETWORKING) << "Found favicon URL" << favIconUrl.toDisplayString() << "in HTML code of webpage" << InternalNetworkAccessManager::removeApiKey(cur.url).toDisplayString();
                    typedUrlStack.push({UrlType::FavIcon, favIconUrl});
                }

                processNextInStack();
            });
        }
    }

};

const QRegularExpression FavIconLocator::Private::invalidChars{QStringLiteral("[^-a-z0-9_]"), QRegularExpression::CaseInsensitiveOption};
const QString FavIconLocator::Private::cacheDirectory{QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/favicons/")};

FavIconLocator::FavIconLocator(const QUrl &webpageUrl, const QUrl &suggestedFavIconUrl, QObject *parent)
        : QObject(parent), d(new Private(this, webpageUrl, suggestedFavIconUrl))
{
    d->processNextInStack();
}

FavIconLocator::~FavIconLocator()
{
    delete d;
}

QIcon FavIconLocator::icon() const
{
    return d->favIcon;
}
