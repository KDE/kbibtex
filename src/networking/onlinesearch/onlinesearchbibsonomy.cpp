/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QBuffer>
#include <QLayout>
#include <QSpinBox>
#include <QLabel>
#include <QNetworkReply>

#include <KLocale>
#include <KDebug>
#include <KIcon>
#include <KLineEdit>
#include <KComboBox>
#include <KMessageBox>
#include <KConfigGroup>

#include "fileimporterbibtex.h"
#include "file.h"
#include "entry.h"
#include "internalnetworkaccessmanager.h"
#include "onlinesearchbibsonomy.h"

class OnlineSearchBibsonomy::OnlineSearchQueryFormBibsonomy : public OnlineSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        comboBoxSearchWhere->setCurrentIndex(configGroup.readEntry(QLatin1String("searchWhere"), 0));
        lineEditSearchTerm->setText(configGroup.readEntry(QLatin1String("searchTerm"), QString()));
        numResultsField->setValue(configGroup.readEntry(QLatin1String("numResults"), 10));
    }

public:
    KComboBox *comboBoxSearchWhere;
    KLineEdit *lineEditSearchTerm;
    QSpinBox *numResultsField;

    OnlineSearchQueryFormBibsonomy(QWidget *widget)
            : OnlineSearchQueryFormAbstract(widget), configGroupName(QLatin1String("Search Engine Bibsonomy")) {
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
        lineEditSearchTerm->setClearButtonShown(true);
        connect(lineEditSearchTerm, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

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

    bool readyToStart() const {
        return !lineEditSearchTerm->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) {
        comboBoxSearchWhere->setCurrentIndex(comboBoxSearchWhere->count() - 1);
        lineEditSearchTerm->setText(authorLastNames(entry).join(" ") + " " + PlainTextValue::text(entry[Entry::ftTitle]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QLatin1String("searchWhere"), comboBoxSearchWhere->currentIndex());
        configGroup.writeEntry(QLatin1String("searchTerm"), lineEditSearchTerm->text());
        configGroup.writeEntry(QLatin1String("numResults"), numResultsField->value());
        config->sync();
    }
};

class OnlineSearchBibsonomy::OnlineSearchBibsonomyPrivate
{
private:
    OnlineSearchBibsonomy *p;

public:
    OnlineSearchQueryFormBibsonomy *form;
    int numSteps, curStep;

    OnlineSearchBibsonomyPrivate(OnlineSearchBibsonomy *parent)
            : p(parent), form(NULL) {
        // nothing
    }

    KUrl buildQueryUrl() {
        if (form == NULL) {
            kWarning() << "Cannot build query url if no form is specified";
            return KUrl();
        }

        QString queryString = p->encodeURL(form->lineEditSearchTerm->text());
        return KUrl("http://www.bibsonomy.org/bib/" + form->comboBoxSearchWhere->itemData(form->comboBoxSearchWhere->currentIndex()).toString() + "/" + queryString + QString("?items=%1").arg(form->numResultsField->value()));
    }

    KUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        QString url = QLatin1String("http://www.bibsonomy.org/bib/");

        bool hasFreeText = !query[queryKeyFreeText].isEmpty();
        bool hasTitle = !query[queryKeyTitle].isEmpty();
        bool hasAuthor = !query[queryKeyAuthor].isEmpty();
        bool hasYear = !query[queryKeyYear].isEmpty();

        QString searchType = "search";
        if (hasAuthor && !hasFreeText && !hasTitle && !hasYear) {
            /// if only the author field is used, a special author search
            /// on BibSonomy can be used
            searchType = "author";
        }

        QStringList queryFragments;
        for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
            queryFragments << p->encodeURL(it.value());
        }

        QString queryString = queryFragments.join("%20");
        url.append(searchType + "/" + queryString + QString("?items=%1").arg(numResults));

        return KUrl(url);
    }

    void sanitizeEntry(QSharedPointer<Entry> entry) {
        /// if entry contains a description field but no abstract,
        /// rename description field to abstract
        const QString ftDescription = QLatin1String("description");
        if (!entry->contains(Entry::ftAbstract) && entry->contains(ftDescription)) {
            Value v = entry->value(QLatin1String("description"));
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
    d->curStep = 0;
    d->numSteps = 1;

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, d->numSteps);
}

void OnlineSearchBibsonomy::startSearch()
{
    m_hasBeenCanceled = false;
    d->curStep = 0;
    d->numSteps = 1;

    QNetworkRequest request(d->buildQueryUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, d->numSteps);

    d->form->saveState();
}

QString OnlineSearchBibsonomy::label() const
{
    return i18n("Bibsonomy");
}

QString OnlineSearchBibsonomy::favIconUrl() const
{
    return QLatin1String("http://www.bibsonomy.org/resources/image/favicon.png");
}

OnlineSearchQueryFormAbstract *OnlineSearchBibsonomy::customWidget(QWidget *parent)
{
    if (d->form == NULL)
        d->form = new OnlineSearchBibsonomy::OnlineSearchQueryFormBibsonomy(parent);
    return d->form;
}

KUrl OnlineSearchBibsonomy::homepage() const
{
    return KUrl("http://www.bibsonomy.org/");
}

void OnlineSearchBibsonomy::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchBibsonomy::downloadDone()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString bibTeXcode = QString::fromUtf8(reply->readAll().data());

        if (!bibTeXcode.isEmpty()) {
            FileImporterBibTeX importer;
            File *bibtexFile = importer.fromString(bibTeXcode);

            bool hasEntries = false;
            if (bibtexFile != NULL) {
                for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                    QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                    hasEntries |= publishEntry(entry);
                }

                if (!hasEntries)
                    kDebug() << "No hits found in" << reply->url().toString();
                emit stoppedSearch(resultNoError);
                emit progress(d->numSteps, d->numSteps);

                delete bibtexFile;
            } else {
                kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
                emit stoppedSearch(resultUnspecifiedError);
            }
        } else {
            /// returned file is empty
            kDebug() << "No hits found in" << reply->url().toString();
            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
