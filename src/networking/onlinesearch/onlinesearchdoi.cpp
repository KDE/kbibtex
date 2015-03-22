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

#include "onlinesearchdoi.h"

#include <QLabel>
#include <QGridLayout>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>

#include <KLineEdit>
#include <KConfigGroup>
#include <KLocale>

#include "kbibtexnamespace.h"
#include "internalnetworkaccessmanager.h"
#include "fileimporterbibtex.h"

class OnlineSearchDOI::OnlineSearchQueryFormDOI : public OnlineSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditDoiNumber->setText(configGroup.readEntry(QLatin1String("doiNumber"), QString()));
    }

public:
    KLineEdit *lineEditDoiNumber;

    OnlineSearchQueryFormDOI(QWidget *widget)
            : OnlineSearchQueryFormAbstract(widget), configGroupName(QLatin1String("Search Engine DOI")) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("DOI:"), this);
        layout->addWidget(label, 0, 0, 1, 1);
        lineEditDoiNumber = new KLineEdit(this);
        layout->addWidget(lineEditDoiNumber, 0, 1, 1, 1);
        lineEditDoiNumber->setClearButtonShown(true);
        connect(lineEditDoiNumber, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

        layout->setRowStretch(1, 100);
        lineEditDoiNumber->setFocus(Qt::TabFocusReason);

        loadState();
    }

    bool readyToStart() const {
        return !lineEditDoiNumber->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) {
        lineEditDoiNumber->setText(PlainTextValue::text(entry[Entry::ftDOI]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("doiNumber"), lineEditDoiNumber->text());
        config->sync();
    }
};


class OnlineSearchDOI::OnlineSearchDOIPrivate
{
private:
    // UNUSED OnlineSearchDOI *p;

public:
    OnlineSearchQueryFormDOI *form;
    int numSteps, curStep;

    OnlineSearchDOIPrivate(OnlineSearchDOI */* UNUSED parent*/)
        : /* UNUSED p(parent),*/ form(NULL) {
        // nothing
    }

    QUrl buildQueryUrl() {
        if (form == NULL) {
            qWarning() << "Cannot build query url if no form is specified";
            return QUrl();
        }

        return QUrl(QLatin1String("http://dx.doi.org/") + form->lineEditDoiNumber->text());
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        Q_UNUSED(numResults)

        if (KBibTeX::doiRegExp.indexIn(query[queryKeyFreeText]) >= 0) {
            return QUrl(QLatin1String("http://dx.doi.org/") + KBibTeX::doiRegExp.cap(0));
        }

        return QUrl();
    }
};


OnlineSearchDOI::OnlineSearchDOI(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchDOIPrivate(this))
{
    // TODO
}

OnlineSearchDOI::~OnlineSearchDOI()
{
    delete d;
}

void OnlineSearchDOI::startSearch()
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 1;

    const QUrl url = d->buildQueryUrl();
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QString("Accept").toLatin1(), QString("text/bibliography; style=bibtex").toLatin1());
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

        emit progress(0, d->numSteps);

        d->form->saveState();
    } else
        delayedStoppedSearch(resultNoError);
}

void OnlineSearchDOI::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 1;

    const QUrl url = d->buildQueryUrl(query, numResults);
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QString("Accept").toLatin1(), QString("text/bibliography; style=bibtex").toLatin1());
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

        emit progress(0, d->numSteps);
    } else
        delayedStoppedSearch(resultNoError);
}

QString OnlineSearchDOI::label() const
{
    return i18n("DOI");
}

OnlineSearchQueryFormAbstract *OnlineSearchDOI::customWidget(QWidget *parent)
{
    if (d->form == NULL)
        d->form = new OnlineSearchDOI::OnlineSearchQueryFormDOI(parent);
    return d->form;
}

QUrl OnlineSearchDOI::homepage() const
{
    return QUrl(QLatin1String("http://dx.doi.org/"));
}

void OnlineSearchDOI::cancel()
{
    OnlineSearchAbstract::cancel();
}

QString OnlineSearchDOI::favIconUrl() const
{
    return QLatin1String("http://dx.doi.org/favicon.ico");
}

void OnlineSearchDOI::downloadDone()
{
    emit progress(++d->curStep, d->numSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++d->numSteps;

            QNetworkRequest request(redirUrl);
            request.setRawHeader(QString("Accept").toLatin1(), QString("text/bibliography; style=bibtex").toLatin1());
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, SIGNAL(finished()), this, SLOT(downloadDone()));
        } else {  /// ensure proper treatment of UTF-8 characters
            const QString bibTeXcode = QString::fromUtf8(reply->readAll().data());

            if (!bibTeXcode.isEmpty()) {
                FileImporterBibTeX importer;
                File *bibtexFile = importer.fromString(bibTeXcode);

                bool hasEntries = false;
                if (bibtexFile != NULL) {
                    for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                        QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                        hasEntries |= publishEntry(entry);
                    }

                    emit stoppedSearch(resultNoError);

                    delete bibtexFile;
                } else {
                    qWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
                    emit stoppedSearch(resultUnspecifiedError);
                }
            } else {
                /// returned file is empty
                emit stoppedSearch(resultNoError);
            }
        }
    } else
        qWarning() << "url was" << reply->url().toString();
}
