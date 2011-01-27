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

#include <KLocale>
#include <KDebug>
#include <KIcon>
#include <KLineEdit>
#include <KComboBox>
#include <kio/job.h>
#include <kio/jobclasses.h>

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

WebSearchBibsonomy::WebSearchBibsonomy(QWidget *parent)
        : WebSearchAbstract(parent), form(NULL)
{
    // nothing
}

void WebSearchBibsonomy::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_buffer.clear();

    m_job = KIO::get(buildQueryUrl(query, numResults));
    connect(m_job, SIGNAL(data(KIO::Job *, const QByteArray &)), this, SLOT(data(KIO::Job*, const QByteArray&)));
    connect(m_job, SIGNAL(result(KJob *)), this, SLOT(jobDone(KJob*)));
}

void WebSearchBibsonomy::startSearch()
{
    m_buffer.clear();

    m_job = KIO::get(buildQueryUrl());
    connect(m_job, SIGNAL(data(KIO::Job *, const QByteArray &)), this, SLOT(data(KIO::Job*, const QByteArray&)));
    connect(m_job, SIGNAL(result(KJob *)), this, SLOT(jobDone(KJob*)));
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
    return new WebSearchBibsonomy::WebSearchQueryFormBibsonomy(parent);
}

KUrl WebSearchBibsonomy::homepage() const
{
    return KUrl("http://www.bibsonomy.org/");
}

KUrl WebSearchBibsonomy::buildQueryUrl()
{
    if (form == NULL)
        return KUrl();

    // FIXME: Is there a need for percent encoding?
    QString queryString = form->lineEditSearchTerm->text();
    // FIXME: Number of results doesn't seem to be supported by BibSonomy
    return KUrl("http://www.bibsonomy.org/bib/" + form->comboBoxSearchWhere->itemData(form->comboBoxSearchWhere->currentIndex()).toString() + "/" + queryString + QString("?.entriesPerPage=%1").arg(form->numResultsField->value()));
}

KUrl WebSearchBibsonomy::buildQueryUrl(const QMap<QString, QString> &query, int numResults)
{
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
        // FIXME: Is there a need for percent encoding?
        queryFragments << it.value();
    }

    QString queryString = queryFragments.join("+");
    // FIXME: Number of results doesn't seem to be supported by BibSonomy
    url.append(searchType + "/" + queryString + QString("?.entriesPerPage=%1").arg(numResults));

    return KUrl(url);
}

void WebSearchBibsonomy::cancel()
{
    m_job->kill(KJob::EmitResult);
}

void WebSearchBibsonomy::data(KIO::Job *job, const QByteArray &data)
{
    Q_UNUSED(job)
    m_buffer.append(data);
}

void WebSearchBibsonomy::jobDone(KJob *job)
{
    if (job->error() == 0) {
        QBuffer buffer(&m_buffer);
        buffer.open(QBuffer::ReadOnly);
        FileImporterBibTeX importer;
        File *bibtexFile = importer.load(&buffer);
        buffer.close();

        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                Entry *entry = dynamic_cast<Entry*>(*it);
                if (entry != NULL)
                    emit foundEntry(entry);

            }
            emit stoppedSearch(bibtexFile->isEmpty() ? resultUnspecifiedError : resultNoError);
            delete bibtexFile;
        } else
            emit stoppedSearch(resultUnspecifiedError);
    } else {
        kWarning() << "Search using " << label() << "failed: " << job->errorString();
        emit stoppedSearch(resultUnspecifiedError);
    }
}
