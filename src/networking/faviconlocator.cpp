/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>

#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

FavIconLocator::FavIconLocator(const QUrl &webpageUrl, QObject *parent)
        : QObject(parent), favIcon(QIcon::fromTheme(QStringLiteral("applications-internet")))
{
    static const QRegularExpression invalidChars(QStringLiteral("[^-a-z0-9_]"), QRegularExpression::CaseInsensitiveOption);
    static const QString cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/favicons/");
    QDir().mkpath(cacheDirectory);
    const QString fileNameStem = cacheDirectory + webpageUrl.toDisplayString().remove(invalidChars);

    /// Try to locate icon in cache first before actually querying the webpage
    static const QStringList fileNameExtensions {QStringLiteral(".png"), QStringLiteral(".ico")};
    for (const QString &extension : fileNameExtensions) {
        const QString fileName = fileNameStem + extension;
        const QFileInfo fi(fileName);
        if (fi.exists(fileName)) {
            if (fi.lastModified().daysTo(QDateTime::currentDateTime()) > 90) {
                /// If icon is other than 90 days, delete it and fetch current one
                QFile::remove(fileName);
            } else {
                favIcon = QIcon(fileName);
                QTimer::singleShot(100, this, [this]() {
                    QMetaObject::invokeMethod(this, "gotIcon", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QIcon, favIcon));
                });
                return;
            }
        }
    }

    QNetworkRequest request(webpageUrl);
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    connect(reply, &QNetworkReply::finished, parent, [this, reply, fileNameStem, webpageUrl]() {
        QUrl favIconUrl;

        if (reply->error() == QNetworkReply::NoError) {
            /// Assume that favicon information is within the first 4K of HTML code
            const QString htmlCode = QString::fromUtf8(reply->readAll()).left(4096);
            /// Some ugly but hopefully fast/flexible/robust HTML code parsing
            int p1 = -1;
            while ((p1 = htmlCode.indexOf(QStringLiteral("<link "), p1 + 5)) > 0) {
                const int p2 = htmlCode.indexOf(QChar('>'), p1 + 5);
                if (p2 > p1) {
                    const int p3 = htmlCode.indexOf(QStringLiteral("rel=\""), p1 + 5);
                    if (p3 > p1 && p3 < p2) {
                        const int p4 = htmlCode.indexOf(QChar('"'), p3 + 5);
                        if (p4 > p3 && p4 < p2) {
                            const QString relValue = htmlCode.mid(p3 + 5, p4 - p3 - 5);
                            if (relValue == QStringLiteral("icon") || relValue == QStringLiteral("shortcut icon")) {
                                const int p5 = htmlCode.indexOf(QStringLiteral("href=\""), p1 + 5);
                                if (p5 > p1 && p5 < p2) {
                                    const int p6 = htmlCode.indexOf(QChar('"'), p5 + 6);
                                    if (p6 > p5 + 5 && p6 < p2) {
                                        QString hrefValue = htmlCode.mid(p5 + 6, p6 - p5 - 6).replace(QLatin1Char('&'), QLatin1String("&amp;")).replace(QLatin1Char('>'), QLatin1String("&gt;")).replace(QLatin1Char('<'), QLatin1String("&lt;"));
                                        /// Do some resolving in case favicon URL in HTML code is relative
                                        favIconUrl = reply->url().resolved(QUrl(hrefValue));
                                        if (favIconUrl.isValid()) {
                                            qCDebug(LOG_KBIBTEX_NETWORKING) << "Found favicon URL" << favIconUrl.toDisplayString() << "in HTML code of webpage" << webpageUrl.toDisplayString();
                                            break;
                                        } else
                                            favIconUrl.clear();
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!favIconUrl.isValid()) {
            favIconUrl = reply->url();
            favIconUrl.setPath(QStringLiteral("/favicon.ico"));
            qCInfo(LOG_KBIBTEX_NETWORKING) << "Could not locate favicon in HTML code for webpage" << webpageUrl.toDisplayString() << ", falling back to" << favIconUrl.toDisplayString();
        }

        QNetworkRequest request(favIconUrl);
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, fileNameStem, favIconUrl, webpageUrl]() {
            if (reply->error() == QNetworkReply::NoError) {
                const QByteArray iconData = reply->readAll();
                if (iconData.size() > 10) {
                    QString extension;
                    if (iconData[1] == 'P' && iconData[2] == 'N' && iconData[3] == 'G') {
                        /// PNG files have string "PNG" at second to fourth byte
                        extension = QStringLiteral(".png");
                    } else if (iconData[0] == static_cast<char>(0x00) && iconData[1] == static_cast<char>(0x00) && iconData[2] == static_cast<char>(0x01) && iconData[3] == static_cast<char>(0x00)) {
                        /// Microsoft Icon have first two bytes always 0x0000,
                        /// third and fourth byte is 0x0001 (for .ico)
                        extension = QStringLiteral(".ico");
                    } else if (iconData[0] == '<') {
                        /// HTML or XML code
                        const QString htmlCode = QString::fromUtf8(iconData);
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Received XML or HTML data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString() << ": " << htmlCode.left(128);
                    } else {
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Favicon is of unknown format: " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                    }

                    if (!extension.isEmpty()) {
                        const QString filename = fileNameStem + extension;

                        QFile iconFile(filename);
                        if (iconFile.open(QFile::WriteOnly)) {
                            iconFile.write(iconData);
                            iconFile.close();
                            qCInfo(LOG_KBIBTEX_NETWORKING) << "Got icon from URL" << favIconUrl.toDisplayString() << "for webpage" << webpageUrl.toDisplayString() << "stored in" << filename;
                            favIcon = QIcon(filename);
                        } else {
                            qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not save icon data from URL" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString() << "to file" << filename;
                        }
                    }
                } else {
                    /// Unlikely that an icon's data is less than 10 bytes,
                    /// must be an error.
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Received invalid icon data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                }
            } else
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not download icon from URL " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString() << ": " << reply->errorString();

            QMetaObject::invokeMethod(this, "gotIcon", Qt::DirectConnection, QGenericReturnArgument(), Q_ARG(QIcon, favIcon));
        });
    });
}

QIcon FavIconLocator::icon() const
{
    return favIcon;
}
