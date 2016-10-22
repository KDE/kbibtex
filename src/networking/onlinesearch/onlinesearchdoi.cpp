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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "onlinesearchdoi.h"

#include <QLabel>
#include <QGridLayout>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <KLineEdit>
#include <KConfigGroup>
#include <KLocalizedString>

#include "kbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "fileimporterbibtex.h"
#include "logging_networking.h"

class OnlineSearchDOI::OnlineSearchQueryFormDOI : public OnlineSearchQueryFormAbstract
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditDoiNumber->setText(configGroup.readEntry(QStringLiteral("doiNumber"), QString()));
    }

public:
    KLineEdit *lineEditDoiNumber;

    OnlineSearchQueryFormDOI(QWidget *widget)
            : OnlineSearchQueryFormAbstract(widget), configGroupName(QStringLiteral("Search Engine DOI")) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("DOI:"), this);
        layout->addWidget(label, 0, 0, 1, 1);
        lineEditDoiNumber = new KLineEdit(this);
        layout->addWidget(lineEditDoiNumber, 0, 1, 1, 1);
        lineEditDoiNumber->setClearButtonEnabled(true);
        connect(lineEditDoiNumber, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormDOI::returnPressed);

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
        configGroup.writeEntry(QStringLiteral("doiNumber"), lineEditDoiNumber->text());
        config->sync();
    }
};


class OnlineSearchDOI::OnlineSearchDOIPrivate
{
private:
    // UNUSED OnlineSearchDOI *p;

public:
    OnlineSearchQueryFormDOI *form;

    OnlineSearchDOIPrivate(OnlineSearchDOI */* UNUSED parent*/)
        : /* UNUSED p(parent),*/ form(NULL) {
        // nothing
    }

    QUrl buildQueryUrl() {
        if (form == NULL) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot build query url if no form is specified";
            return QUrl();
        }

        return QUrl(QStringLiteral("http://dx.doi.org/") + form->lineEditDoiNumber->text());
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        Q_UNUSED(numResults)

        if (KBibTeX::doiRegExp.indexIn(query[queryKeyFreeText]) >= 0) {
            return QUrl(QStringLiteral("http://dx.doi.org/") + KBibTeX::doiRegExp.cap(0));
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

void OnlineSearchDOI::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl();
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("Accept"), QByteArray("text/bibliography; style=bibtex"));
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchDOI::downloadDone);

        d->form->saveState();
    } else
        delayedStoppedSearch(resultNoError);
}

void OnlineSearchDOI::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl(query, numResults);
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("Accept"), QByteArray("text/bibliography; style=bibtex"));
        QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
        InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchDOI::downloadDone);
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
    return QUrl(QStringLiteral("http://dx.doi.org/"));
}

QString OnlineSearchDOI::favIconUrl() const
{
    return QStringLiteral("http://dx.doi.org/favicon.ico");
}

void OnlineSearchDOI::downloadDone()
{
    emit progress(++curStep, numSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);
            request.setRawHeader(QByteArray("Accept"), QByteArray("text/bibliography; style=bibtex"));
            QNetworkReply *newReply = InternalNetworkAccessManager::self()->get(request);
            InternalNetworkAccessManager::self()->setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchDOI::downloadDone);
        } else {  /// ensure proper treatment of UTF-8 characters
            const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

            if (!bibTeXcode.isEmpty()) {
                FileImporterBibTeX importer;
                File *bibtexFile = importer.fromString(bibTeXcode);

                bool hasEntries = false;
                if (bibtexFile != NULL) {
                    for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                        QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                        hasEntries |= publishEntry(entry);
                    }

                    stopSearch(resultNoError);

                    delete bibtexFile;
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toDisplayString();
                    stopSearch(resultUnspecifiedError);
                }
            } else {
                /// returned file is empty
                stopSearch(resultNoError);
            }
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}

#include "onlinesearchdoi.moc"
