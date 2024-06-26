/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_NETWORKING_ONLINESEARCHABSTRACT_H
#define KBIBTEX_NETWORKING_ONLINESEARCHABSTRACT_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QNetworkReply>
#ifdef HAVE_QTWIDGETS
#include <QWidget>
#endif // HAVE_QTWIDGETS
#include <QMetaType>
#include <QIcon>
#include <QUrl>

#include <Entry>

#ifdef HAVE_KF
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF

class QNetworkReply;
class QNetworkRequest;
class QListWidgetItem;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchAbstract : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit OnlineSearchAbstract(QObject *parent);

#ifdef HAVE_QTWIDGETS
    class Form;
#endif // HAVE_QTWIDGETS

    enum class QueryKey {FreeText = 1, Title = 2, Author = 3, Year = 4};

    static const int resultCancelled;
    static const int resultNoError;
    static const int resultUnspecifiedError;
    static const int resultAuthorizationRequired;
    static const int resultNetworkError;
    static const int resultInvalidArguments;

#ifdef HAVE_QTWIDGETS
    virtual void startSearchFromForm();
#endif // HAVE_QTWIDGETS
    virtual void startSearch(const QMap<QueryKey, QString> &query, int numResults) = 0;
    virtual QString label() const = 0;
    QString name();
#ifdef HAVE_QTWIDGETS
    virtual QIcon icon(QListWidgetItem *listWidgetItem);
    virtual OnlineSearchAbstract::Form *customWidget(QWidget *parent);
#endif // HAVE_QTWIDGETS
    virtual QUrl homepage() const = 0;
    virtual bool busy() const;

public Q_SLOTS:
    void cancel();

protected:
    QObject *m_parent;
    bool m_hasBeenCanceled;

    int numSteps, curStep;

    /**
     * Split a string along spaces, but keep text in quotation marks together
     */
    static QStringList splitRespectingQuotationMarks(const QString &text);

    /**
    * Will check for common problems with downloads via QNetworkReply. It will return true
    * if there is no problem and you may process this job result. If there is a problem,
    * this function will notify the user if necessary (KMessageBox), emit a
    * "stoppedSearch" signal (by invoking "stopSearch"), and return false.
    * @see handleErrors(KJob*)
    */
    bool handleErrors(QNetworkReply *reply);

    /**
    * Will check for common problems with downloads via QNetworkReply. It will return true
    * if there is no problem and you may process this job result. If there is a problem,
    * this function will notify the user if necessary (KMessageBox), emit a
    * "stoppedSearch" signal (by invoking "stopSearch"), and return false.
    * @see handleErrors(KJob*)
    * @param reply The reply the function will handle errors for
    * @param newUrl will be set to the new URL if reply contains a redirection, otherwise reply's original URL
    */
    bool handleErrors(QNetworkReply *reply, QUrl &newUrl, const QSet<const QNetworkReply::NetworkError> &ignoredErrors = QSet<const QNetworkReply::NetworkError>({QNetworkReply::NoError}));

    /**
     * Encode a text to be HTTP URL save, e.g. replace '=' by '%3D'.
     */
    static QString encodeURL(QString rawText);

    static QString decodeURL(QString rawText);

    static QString deHTMLify(const QString &input);
    static QByteArray htmlEntityToUnicode(const QByteArray &input);

    static QString monthToMacroKeyText(const QString &rawText);

    QMultiMap<QString, QString> formParameters(const QString &htmlText, int startPos) const;

    /**
     * Delay sending of stop signal by a few milliseconds.
     * Necessary if search stops (is cancelled) already in one
     * of the startSearch functions.
     */
    void delayedStoppedSearch(int returnCode);

    /**
     * Correct the most common problems encountered in fetched entries.
     * This function should be specialized in each descendant of this class.
     * @param entry Entry to sanitize
     */
    virtual void sanitizeEntry(QSharedPointer<Entry> entry);

    /**
     * Perform the following steps: (1) sanitize entry, (2) add name
     * of search engine that found the entry, (3) send it back to search
     * result list.
     * Returns true if a valid entry was passed to this function and all
     * steps could be performed.
     * @param entry Entry to publish
     * @return returns true if a valid entry was passed to this function and all steps could be performed.
     */
    bool publishEntry(QSharedPointer<Entry> entry);

    void stopSearch(int errorCode);

    /**
     * Allows an online search to notify about a change of its busy state,
     * i.e. that the public function @see busy may return a different value
     * than before. If the actual busy state has changed compared to previous
     * invocations of this function, the signal @see busyChanged will be
     * emitted.
     * This function here may be called both on changes from active to
     * inactive as well as vice versa.
     */
    void refreshBusyProperty();

#ifdef HAVE_KF
    /**
     * @brief Send a visual notification to the desktop, similar to KNotification
     * @param text Message to be shown
     * @param title Title of the message, such as which OnlineSearch engine
     * @param timeout time after which the message will automatically disappear (in seconds)
     * @param icon Name of icon to be shown
     */
    void sendVisualNotification(const QString &text, const QString &title, int timeout = 10, const QString &icon = QStringLiteral("kbibtex"));
#endif // HAVE_KF

private:
    bool m_previousBusyState;
    QString m_name;
    static const char *httpUnsafeChars;
#ifdef HAVE_QTWIDGETS
    QMap<QNetworkReply *, QListWidgetItem *> m_iconReplyToListWidgetItem;
#endif // HAVE_QTWIDGETS
    int m_delayedStoppedSearchReturnCode;

    QString htmlAttribute(const QString &htmlCode, const int startPos, const QString &attribute) const;
    bool htmlAttributeIsSelected(const QString &htmlCode, const int startPos, const QString &attribute) const;

Q_SIGNALS:
    void foundEntry(QSharedPointer<Entry>);
    void stoppedSearch(int);
    void progress(int, int);
    void busyChanged();
};

#ifdef HAVE_QTWIDGETS
/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OnlineSearchAbstract::Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent);
    ~Form();

    virtual bool readyToStart() const = 0;
    virtual void copyFromEntry(const Entry &) = 0;

Q_SIGNALS:
    void returnPressed();

protected:
    class Private;
    Private *d;
};
#endif // HAVE_QTWIDGETS

#endif // KBIBTEX_NETWORKING_ONLINESEARCHABSTRACT_H
