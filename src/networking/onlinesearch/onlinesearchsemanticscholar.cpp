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

#include "onlinesearchsemanticscholar.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QRegularExpression>
#ifdef HAVE_QTWIDGETS
#include <QLineEdit>
#include <QFormLayout>
#endif // HAVE_QTWIDGETS

#ifdef HAVE_KF
#include <KConfigGroup>
#include <KLocalizedString>
#endif // HAVE_KF

#include <KBibTeX>
#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "onlinesearchabstract_p.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
class OnlineSearchSemanticScholar::OnlineSearchQueryFormSemanticScholar : public OnlineSearchAbstract::Form
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(d->config, configGroupName);
        lineEditPaperReference->setText(configGroup.readEntry(QStringLiteral("paperReference"), QString()));
    }

public:
    QLineEdit *lineEditPaperReference;

    OnlineSearchQueryFormSemanticScholar(QWidget *widget)
            : OnlineSearchAbstract::Form(widget), configGroupName(QStringLiteral("Search Engine Semantic Scholar")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        lineEditPaperReference = new QLineEdit(this);
        lineEditPaperReference->setClearButtonEnabled(true);
        lineEditPaperReference->setFocus(Qt::TabFocusReason);
        layout->addRow(i18n("Paper Reference:"), lineEditPaperReference);
        connect(lineEditPaperReference, &QLineEdit::returnPressed, this, &OnlineSearchQueryFormSemanticScholar::returnPressed);

        loadState();
    }

    bool readyToStart() const override {
        return !lineEditPaperReference->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) override {
        lineEditPaperReference->setText(d->guessFreeText(entry));
    }

    void saveState() {
        KConfigGroup configGroup(d->config, configGroupName);
        configGroup.writeEntry(QStringLiteral("paperReference"), lineEditPaperReference->text());
        d->config->sync();
    }
};
#endif // HAVE_QTWIDGETS


class OnlineSearchSemanticScholar::OnlineSearchSemanticScholarPrivate
{
public:
#ifdef HAVE_QTWIDGETS
    OnlineSearchQueryFormSemanticScholar *form;
#endif // HAVE_QTWIDGETS

    OnlineSearchSemanticScholarPrivate(OnlineSearchSemanticScholar *parent)
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

        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(form->lineEditPaperReference->text());
        if (doiRegExpMatch.hasMatch())
            return QUrl(QStringLiteral("https://api.semanticscholar.org/v1/paper/") + doiRegExpMatch.captured(QStringLiteral("doi")));
        else {
            const QRegularExpressionMatch arXivRegExpMatch = KBibTeX::arXivRegExp.match(form->lineEditPaperReference->text());
            if (arXivRegExpMatch.hasMatch())
                return QUrl(QStringLiteral("https://api.semanticscholar.org/v1/paper/arXiv:") + arXivRegExpMatch.captured(QStringLiteral("arxiv")));
        }

        return QUrl();
    }
#endif // HAVE_QTWIDGETS

    QUrl buildQueryUrl(const QMap<QueryKey, QString> &query, int numResults) {
        Q_UNUSED(numResults)

        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(query[QueryKey::FreeText]);
        if (doiRegExpMatch.hasMatch())
            return QUrl(QStringLiteral("https://api.semanticscholar.org/v1/paper/") + doiRegExpMatch.captured(QStringLiteral("doi")));
        else {
            const QRegularExpressionMatch arXivRegExpMatch = KBibTeX::arXivRegExp.match(query[QueryKey::FreeText]);
            if (arXivRegExpMatch.hasMatch())
                return QUrl(QStringLiteral("https://api.semanticscholar.org/v1/paper/arXiv:") + arXivRegExpMatch.captured(QStringLiteral("arxiv")));
        }
        return QUrl();
    }

    Entry *entryFromJsonObject(const QJsonObject &object) const {
        const QString title = object.value(QStringLiteral("title")).toString();
        const QString paperId = object.value(QStringLiteral("paperId")).toString();
        const int year = object.value(QStringLiteral("year")).toInt(-1);
        /// Basic sanity check
        if (title.isEmpty() || paperId.isEmpty() || year < 1700)
            return nullptr;

        Entry *entry = new Entry(Entry::etMisc, QStringLiteral("SemanticScholar:") + paperId);
        entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(title)));
        entry->insert(QStringLiteral("x-paperId"), Value() << QSharedPointer<VerbatimText>(new VerbatimText(paperId)));
        entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QString::number(year))));

        const QString doi = object.value(QStringLiteral("doi")).toString();
        const QRegularExpressionMatch doiRegExpMatch = KBibTeX::doiRegExp.match(doi);
        if (doiRegExpMatch.hasMatch())
            entry->insert(Entry::ftDOI, Value() << QSharedPointer<VerbatimText>(new VerbatimText(doiRegExpMatch.captured(QStringLiteral("doi")))));

        const QString arxivId = object.value(QStringLiteral("arxivId")).toString();
        const QRegularExpressionMatch arXivRegExpMatch = KBibTeX::arXivRegExp.match(arxivId);
        if (arXivRegExpMatch.hasMatch())
            entry->insert(QStringLiteral("eprint"), Value() << QSharedPointer<VerbatimText>(new VerbatimText(arXivRegExpMatch.captured(QStringLiteral("arxiv")))));

        const QJsonArray authorArray = object.value(QStringLiteral("authors")).toArray();
        Value authors;
        for (const QJsonValue &author : authorArray) {
            const QString name = author.toObject().value(QStringLiteral("name")).toString();
            if (!name.isEmpty()) {
                QSharedPointer<Person> person = FileImporterBibTeX::personFromString(name);
                if (!person.isNull())
                    authors.append(person);
            }
        }
        if (!authors.isEmpty())
            entry->insert(Entry::ftAuthor, authors);

        return entry;
    }
};


OnlineSearchSemanticScholar::OnlineSearchSemanticScholar(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchSemanticScholarPrivate(this))
{
    /// nothing
}

OnlineSearchSemanticScholar::~OnlineSearchSemanticScholar()
{
    delete d;
}

#ifdef HAVE_QTWIDGETS
void OnlineSearchSemanticScholar::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl();
    if (url.isValid()) {
        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchSemanticScholar::downloadDone);

        d->form->saveState();
    } else
        delayedStoppedSearch(resultNoError);

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchSemanticScholar::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    const QUrl url = d->buildQueryUrl(query, numResults);
    if (url.isValid()) {
        QNetworkRequest request(url);
        QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
        InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
        connect(reply, &QNetworkReply::finished, this, &OnlineSearchSemanticScholar::downloadDone);
    } else
        delayedStoppedSearch(resultNoError);

    refreshBusyProperty();
}

QString OnlineSearchSemanticScholar::label() const
{
    return i18n("Semantic Scholar");
}

#ifdef HAVE_QTWIDGETS
OnlineSearchAbstract::Form *OnlineSearchSemanticScholar::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchSemanticScholar::OnlineSearchQueryFormSemanticScholar(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchSemanticScholar::homepage() const
{
    return QUrl(QStringLiteral("https://www.semanticscholar.org/"));
}

void OnlineSearchSemanticScholar::downloadDone()
{
    Q_EMIT progress(++curStep, numSteps);
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    QUrl redirUrl;
    if (handleErrors(reply, redirUrl)) {
        if (redirUrl.isValid()) {
            /// redirection to another url
            ++numSteps;

            QNetworkRequest request(redirUrl);
            QNetworkReply *newReply = InternalNetworkAccessManager::instance().get(request);
            InternalNetworkAccessManager::instance().setNetworkReplyTimeout(newReply);
            connect(newReply, &QNetworkReply::finished, this, &OnlineSearchSemanticScholar::downloadDone);
        } else {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                if (document.isObject()) {
                    Entry *entry = d->entryFromJsonObject(document.object());
                    if (entry != nullptr) {
                        publishEntry(QSharedPointer<Entry>(entry));
                        stopSearch(resultNoError);
                    } else {
                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Semantic Scholar: Data could not be interpreted as a bibliographic entry";
                        stopSearch(resultUnspecifiedError);
                    }
                } else {
                    qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Semantic Scholar: Document is not an object";
                    stopSearch(resultUnspecifiedError);
                }
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "Problem with JSON data from Semantic Scholar: " << parseError.errorString();
                stopSearch(resultUnspecifiedError);
            }
        }
    }

    refreshBusyProperty();
}

#include "onlinesearchsemanticscholar.moc"
