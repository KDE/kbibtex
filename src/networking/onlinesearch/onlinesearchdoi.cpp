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

#include "onlinesearchdoi.h"

#ifdef HAVE_QTWIDGETS
#include <QLabel>
#include <QGridLayout>
#endif // HAVE_QTWIDGETS
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>

#ifdef HAVE_QTWIDGETS
#include <KLineEdit>
#include <KConfigGroup>
#endif // HAVE_QTWIDGETS
#include <KLocalizedString>

#include "kbibtex.h"
#include "internalnetworkaccessmanager.h"
#include "fileimporterbibtex.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
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

    bool readyToStart() const override {
        return !lineEditDoiNumber->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) override {
        lineEditDoiNumber->setText(PlainTextValue::text(entry[Entry::ftDOI]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QStringLiteral("doiNumber"), lineEditDoiNumber->text());
        config->sync();
    }
};
#endif // HAVE_QTWIDGETS


class OnlineSearchDOI::OnlineSearchDOIPrivate
{
public:
#ifdef HAVE_QTWIDGETS
    OnlineSearchQueryFormDOI *form;
#endif // HAVE_QTWIDGETS

    OnlineSearchDOIPrivate(OnlineSearchDOI *parent)
#ifdef HAVE_QTWIDGETS
            : form(nullptr)
#endif // HAVE_QTWIDGETS
    {
        Q_UNUSED(parent)
    }

#ifdef HAVE_QTWIDGETS
    QUrl buildQueryUrl() {
        if (form == nullptr) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot build query url if no form is specified";
            return QUrl();
        }

        return QUrl(QStringLiteral("https://dx.doi.org/") + form->lineEditDoiNumber->text());
    }
#endif // HAVE_QTWIDGETS

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        Q_UNUSED(numResults)

        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(query[queryKeyFreeText]);
        if (doiRegExpMatch.hasMatch()) {
            return QUrl(QStringLiteral("https://dx.doi.org/") + doiRegExpMatch.captured(0));
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

#ifdef HAVE_QTWIDGETS
void OnlineSearchDOI::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl();
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("Accept"), QByteArray("text/bibliography; style=bibtex"));
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchDOI::downloadDone);

        d->form->saveState();
    } else
        delayedStoppedSearch(resultNoError);

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchDOI::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl(query, numResults);
    if (url.isValid()) {
        QNetworkRequest request(url);
        request.setRawHeader(QByteArray("Accept"), QByteArray("text/bibliography; style=bibtex"));
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchDOI::downloadDone);

        refreshBusyProperty();
    } else
        delayedStoppedSearch(resultNoError);
}

QString OnlineSearchDOI::label() const
{
    return i18n("DOI");
}

#ifdef HAVE_QTWIDGETS
OnlineSearchQueryFormAbstract *OnlineSearchDOI::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchDOI::OnlineSearchQueryFormDOI(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchDOI::homepage() const
{
    return QUrl(QStringLiteral("https://dx.doi.org/"));
}

QString OnlineSearchDOI::favIconUrl() const
{
    return QStringLiteral("https://dx.doi.org/favicon.ico");
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
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchDOI::downloadDone);
        } else {  /// ensure proper treatment of UTF-8 characters
            const QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

            if (!bibTeXcode.isEmpty()) {
                FileImporterBibTeX importer(this);
                File *bibtexFile = importer.fromString(bibTeXcode);

                bool hasEntries = false;
                if (bibtexFile != nullptr) {
                    for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                        hasEntries |= publishEntry(entry);
                    }

                    stopSearch(resultNoError);

                    delete bibtexFile;
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
                    stopSearch(resultUnspecifiedError);
                }
            } else {
                /// returned file is empty
                stopSearch(resultNoError);
            }
        }
    }

    refreshBusyProperty();
}

#include "onlinesearchdoi.moc"
