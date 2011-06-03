/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <fileimporterbibtex.h>
#include <file.h>
#include <entry.h>
#include "websearchbibsonomy.h"

class WebSearchBibsonomy::WebSearchQueryFormBibsonomy : public WebSearchQueryFormAbstract
{
public:
    KComboBox *comboBoxSearchWhere;
    KLineEdit *lineEditSearchTerm;
    QSpinBox *numResultsField;

    WebSearchQueryFormBibsonomy(QWidget *widget)
            : WebSearchQueryFormAbstract(widget) {
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
    }

    virtual bool readyToStart() const {
        return !lineEditSearchTerm->text().isEmpty();
    }
};

class WebSearchBibsonomy::WebSearchBibsonomyPrivate
{
private:
    WebSearchBibsonomy *p;

public:
    WebSearchQueryFormBibsonomy *form;

    WebSearchBibsonomyPrivate(WebSearchBibsonomy *parent)
            : p(parent), form(NULL) {
        // nothing
    }

    KUrl buildQueryUrl() {
        if (form == NULL) {
            kWarning() << "Cannot build query url if no form is specified";
            return KUrl();
        }

        QString queryString = p->encodeURL(form->lineEditSearchTerm->text());
        // FIXME: Number of results doesn't seem to be supported by BibSonomy
        return KUrl("http://www.bibsonomy.org/bib/" + form->comboBoxSearchWhere->itemData(form->comboBoxSearchWhere->currentIndex()).toString() + "/" + queryString + QString("?.entriesPerPage=%1").arg(form->numResultsField->value()));
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
        // FIXME: Number of results doesn't seem to be supported by BibSonomy
        url.append(searchType + "/" + queryString + QString("?.entriesPerPage=%1").arg(numResults));

        return KUrl(url);
    }
};

WebSearchBibsonomy::WebSearchBibsonomy(QWidget *parent)
        : WebSearchAbstract(parent), d(new WebSearchBibsonomyPrivate(this))
{
    // nothing
}

void WebSearchBibsonomy::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    QNetworkReply *reply = networkAccessManager()->get(QNetworkRequest(d->buildQueryUrl(query, numResults)));
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));
}

void WebSearchBibsonomy::startSearch()
{
    m_hasBeenCanceled = false;

    QNetworkReply *reply = networkAccessManager()->get(QNetworkRequest(d->buildQueryUrl()));
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));
}

QString WebSearchBibsonomy::label() const
{
    return i18n("Bibsonomy");
}

QString WebSearchBibsonomy::favIconUrl() const
{
    return QLatin1String("http://www.bibsonomy.org/resources/image/favicon.png");
}

WebSearchQueryFormAbstract* WebSearchBibsonomy::customWidget(QWidget *parent)
{
    d->form = new WebSearchBibsonomy::WebSearchQueryFormBibsonomy(parent);
    return d->form;
}

KUrl WebSearchBibsonomy::homepage() const
{
    return KUrl("http://www.bibsonomy.org/");
}

void WebSearchBibsonomy::cancel()
{
    WebSearchAbstract::cancel();
}

void WebSearchBibsonomy::downloadDone()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(sender());

    if (handleErrors(reply)) {
        QString bibTeXcode = reply->readAll();

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL) {
                    emit foundEntry(entry);
                    hasEntries = true;
                }

            }

            if (!hasEntries)
                kDebug() << "No hits found in" << reply->url().toString();
            emit stoppedSearch(resultNoError);

            delete bibtexFile;
        } else {
            kWarning() << "No valid BibTeX file results returned on request on" << reply->url().toString();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        kDebug() << "url was" << reply->url().toString();
}
