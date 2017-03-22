/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "onlinesearchbibsonomy.h"

#include <QBuffer>
#include <QLayout>
#include <QSpinBox>
#include <QLabel>
#include <QNetworkReply>
#include <QIcon>

#include <KLocalizedString>
#include <KComboBox>
#include <KMessageBox>
#include <KConfigGroup>
#include <KLineEdit>

#include "fileimporterbibtex.h"
#include "file.h"
#include "entry.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchBibsonomy::OnlineSearchQueryFormBibsonomy : public OnlineSearchQueryFormAbstract
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        comboBoxSearchWhere->setCurrentIndex(configGroup.readEntry(QStringLiteral("searchWhere"), 0));
        lineEditSearchTerm->setText(configGroup.readEntry(QStringLiteral("searchTerm"), QString()));
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
    }

public:
    KComboBox *comboBoxSearchWhere;
    KLineEdit *lineEditSearchTerm;
    QSpinBox *numResultsField;

    OnlineSearchQueryFormBibsonomy(QWidget *widget)
            : OnlineSearchQueryFormAbstract(widget), configGroupName(QStringLiteral("Search Engine Bibsonomy")) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);

        comboBoxSearchWhere = new KComboBox(false, this);
        layout->addWidget(comboBoxSearchWhere, 0, 0, 1, 1);
        comboBoxSearchWhere->addItem(i18n("Tag"), "tag");
        comboBoxSearchWhere->addItem(i18n("User"), "user");
        comboBoxSearchWhere->addItem(i18n("Group"), "group");
        comboBoxSearchWhere->addItem(i18n("Author"), "author");
        comboBoxSearchWhere->addItem(i18n("Concept"), "concept/tag");
        comboBoxSearchWhere->addItem(i18n("BibTeX Key"), "bibtexkey");
        comboBoxSearchWhere->addItem(i18n("Everywhere"), "search");
        comboBoxSearchWhere->setCurrentIndex(comboBoxSearchWhere->count() - 1);

        lineEditSearchTerm = new KLineEdit(this);
        layout->addWidget(lineEditSearchTerm, 0, 1, 1, 1);
        lineEditSearchTerm->setClearButtonEnabled(true);
        connect(lineEditSearchTerm, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormBibsonomy::returnPressed);

        QLabel *label = new QLabel(i18n("Number of Results:"), this);
        layout->addWidget(label, 1, 0, 1, 1);
        numResultsField = new QSpinBox(this);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(20);
        layout->addWidget(numResultsField, 1, 1, 1, 1);
        label->setBuddy(numResultsField);

        layout->setRowStretch(2, 100);
        lineEditSearchTerm->setFocus(Qt::TabFocusReason);

        loadState();
    }

    bool readyToStart() const override {
        return !lineEditSearchTerm->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) override {
        comboBoxSearchWhere->setCurrentIndex(comboBoxSearchWhere->count() - 1);
        lineEditSearchTerm->setText(authorLastNames(entry).join(QStringLiteral(" ")) + QLatin1Char(' ') + PlainTextValue::text(entry[Entry::ftTitle]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QStringLiteral("searchWhere"), comboBoxSearchWhere->currentIndex());
        configGroup.writeEntry(QStringLiteral("searchTerm"), lineEditSearchTerm->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        config->sync();
    }
};

class OnlineSearchBibsonomy::OnlineSearchBibsonomyPrivate
{
private:
    OnlineSearchBibsonomy *p;

public:
    OnlineSearchQueryFormBibsonomy *form;

    OnlineSearchBibsonomyPrivate(OnlineSearchBibsonomy *parent)
            : p(parent), form(nullptr) {
        // nothing
    }

    QUrl buildQueryUrl() {
        if (form == nullptr) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot build query url if no form is specified";
            return QUrl();
        }

        QString queryString = p->encodeURL(form->lineEditSearchTerm->text());
        return QUrl(QStringLiteral("http://www.bibsonomy.org/bib/") + form->comboBoxSearchWhere->itemData(form->comboBoxSearchWhere->currentIndex()).toString() + "/" + queryString + QString(QStringLiteral("?items=%1")).arg(form->numResultsField->value()));
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QString url = QStringLiteral("http://www.bibsonomy.org/bib/");

        bool hasFreeText = !query[queryKeyFreeText].isEmpty();
        bool hasTitle = !query[queryKeyTitle].isEmpty();
        bool hasAuthor = !query[queryKeyAuthor].isEmpty();
        bool hasYear = !query[queryKeyYear].isEmpty();

        QString searchType = QStringLiteral("search");
        if (hasAuthor && !hasFreeText && !hasTitle && !hasYear) {
            /// if only the author field is used, a special author search
            /// on BibSonomy can be used
            searchType = QStringLiteral("author");
        }

        QStringList queryFragments;
        for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
            queryFragments << p->encodeURL(it.value());
        }

        QString queryString = queryFragments.join(QStringLiteral("%20"));
        url.append(searchType + QLatin1Char('/') + queryString + QString(QStringLiteral("?items=%1")).arg(numResults));

        return QUrl(url);
    }

    void sanitizeEntry(QSharedPointer<Entry> entry) {
        /// if entry contains a description field but no abstract,
        /// rename description field to abstract
        const QString ftDescription = QStringLiteral("description");
        if (!entry->contains(Entry::ftAbstract) && entry->contains(ftDescription)) {
            Value v = entry->value(QStringLiteral("description"));
            entry->insert(Entry::ftAbstract, v);
            entry->remove(ftDescription);
        }
    }
};

OnlineSearchBibsonomy::OnlineSearchBibsonomy(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchBibsonomyPrivate(this))
{
    // nothing
}

OnlineSearchBibsonomy::~OnlineSearchBibsonomy()
{
    delete d;
}

void OnlineSearchBibsonomy::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchBibsonomy::downloadDone);
}

void OnlineSearchBibsonomy::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchBibsonomy::downloadDone);

    emit progress(0, numSteps);
}

QString OnlineSearchBibsonomy::label() const
{
    return i18n("Bibsonomy");
}

QString OnlineSearchBibsonomy::favIconUrl() const
{
    return QStringLiteral("http://www.bibsonomy.org/resources/image/favicon.png");
}

OnlineSearchQueryFormAbstract *OnlineSearchBibsonomy::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchBibsonomy::OnlineSearchQueryFormBibsonomy(parent);
    return d->form;
}

QUrl OnlineSearchBibsonomy::homepage() const
{
    return QUrl(QStringLiteral("http://www.bibsonomy.org/"));
}

void OnlineSearchBibsonomy::downloadDone()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().constData());

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer(this);
            const File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != nullptr) {
                for (const auto &element : const_cast<const File &>(*bibtexFile)) {
                    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                    hasEntries |= publishEntry(entry);
                }

                stopSearch(resultNoError);
                emit progress(curStep = numSteps, numSteps);

                delete bibtexFile;
            } else {
                qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toDisplayString();
                stopSearch(resultUnspecifiedError);
            }
        } else {
            /// returned file is empty
            stopSearch(resultNoError);
            emit progress(curStep = numSteps, numSteps);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toDisplayString();
}

#include "onlinesearchbibsonomy.moc"
