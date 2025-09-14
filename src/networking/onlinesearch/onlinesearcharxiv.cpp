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

#include "onlinesearcharxiv.h"

#include <QNetworkReply>
#include <QRegularExpression>
#ifdef HAVE_QTWIDGETS
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextStream>
#endif // HAVE_QTWIDGETS
#include <QXmlStreamReader>

#ifdef HAVE_KF
#include <KMessageBox>
#include <KConfigGroup>
#include <KLocalizedString>
#else // HAVE_KF
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF

#include <KBibTeX>
#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "onlinesearchabstract_p.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
class OnlineSearchArXiv::Form : public OnlineSearchAbstract::Form
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(d->config, configGroupName);
        lineEditFreeText->setText(configGroup.readEntry(QStringLiteral("freeText"), QString()));
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
    }

public:
    QLineEdit *lineEditFreeText;
    QSpinBox *numResultsField;

    Form(QWidget *parent)
            : OnlineSearchAbstract::Form(parent), configGroupName(QStringLiteral("Search Engine arXiv.org")) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        QLabel *label = new QLabel(i18n("Free text:"), this);
        layout->addWidget(label, 0, 0, 1, 1);
        lineEditFreeText = new QLineEdit(this);
        lineEditFreeText->setClearButtonEnabled(true);
        lineEditFreeText->setFocus(Qt::TabFocusReason);
        layout->addWidget(lineEditFreeText, 0, 1, 1, 1);
        label->setBuddy(lineEditFreeText);
        connect(lineEditFreeText, &QLineEdit::returnPressed, this, &OnlineSearchArXiv::Form::returnPressed);

        label = new QLabel(i18n("Number of Results:"), this);
        layout->addWidget(label, 1, 0, 1, 1);
        numResultsField = new QSpinBox(this);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(100);
        numResultsField->setValue(20);
        layout->addWidget(numResultsField, 1, 1, 1, 1);
        label->setBuddy(numResultsField);

        layout->setRowStretch(2, 100);

        loadState();
    }

    bool readyToStart() const override {
        return !lineEditFreeText->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) override {
        lineEditFreeText->setText(d->authorLastNames(entry).join(QStringLiteral(" ")) + u' ' + PlainTextValue::text(entry[Entry::ftTitle]));
    }

    void saveState() {
        KConfigGroup configGroup(d->config, configGroupName);
        configGroup.writeEntry(QStringLiteral("freeText"), lineEditFreeText->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        d->config->sync();
    }
};
#endif // HAVE_QTWIDGETS

class OnlineSearchArXiv::OnlineSearchArXivPrivate
{
public:
    const QString arXivQueryBaseUrl;
#ifdef HAVE_QTWIDGETS
    OnlineSearchArXiv::Form *form;
#endif // HAVE_QTWIDGETS

    OnlineSearchArXivPrivate(OnlineSearchArXiv *)
            : arXivQueryBaseUrl(QStringLiteral("https://export.arxiv.org/api/query?"))
#ifdef HAVE_QTWIDGETS
        , form(nullptr)
#endif // HAVE_QTWIDGETS
    {
        // nothing
    }

#ifdef HAVE_QTWIDGETS
    QUrl buildQueryUrl() {
        /// format search terms
        const auto respectingQuotationMarks = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditFreeText->text());
        QStringList queryFragments;
        queryFragments.reserve(respectingQuotationMarks.size());
        for (const QString &queryFragment : respectingQuotationMarks)
            queryFragments.append(OnlineSearchAbstract::encodeURL(queryFragment));
        return QUrl(QString(QStringLiteral("%1search_query=all:\"%3\"&start=0&max_results=%2")).arg(arXivQueryBaseUrl).arg(form->numResultsField->value()).arg(queryFragments.join(QStringLiteral("\"+AND+all:\"")))); ///< join search terms with an AND operation
    }
#endif // HAVE_QTWIDGETS

    QUrl buildQueryUrl(const QMap<QueryKey, QString> &query, int numResults) {
        /// format search terms
        QStringList queryFragments;
        for (QMap<QueryKey, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
            const auto respectingQuotationMarks = OnlineSearchAbstract::splitRespectingQuotationMarks(it.value());
            for (const auto &queryFragment : respectingQuotationMarks)
                queryFragments.append(OnlineSearchAbstract::encodeURL(queryFragment));
        }
        return QUrl(QString(QStringLiteral("%1search_query=all:\"%3\"&start=0&max_results=%2")).arg(arXivQueryBaseUrl).arg(numResults).arg(queryFragments.join(QStringLiteral("\"+AND+all:\"")))); ///< join search terms with an AND operation
    }

    void evaluateJournal(const QString &journal, QSharedPointer<Entry> &entry) {
        // Nothing to do on empty journal text
        if (journal.isEmpty()) return;
        else entry->remove(Entry::ftJournal);

        /**
         * TODO examples
         *  - https://arxiv.org/abs/1111.5338
         *    Eur. Phys. J. H 36, 183-201 (2011)
         *  - https://arxiv.org/abs/1111.5348
         *    IJCSI International Journal of Computer Science Issues, Vol. 8, Issue 3, No. 1, 2011, 224-229
         *  - https://arxiv.org/abs/1110.3379
         *    IJCSI International Journal of Computer Science Issues, Vol. 8, Issue 5, No 3, September 2011 ISSN (Online): 1694-0814
         *  - https://arxiv.org/abs/1102.5769
         *    The International Journal of Multimedia & Its Applications, 3(1), 2011
         *  - https://arxiv.org/abs/1003.3022
         *    American Journal of Physics -- April 2010 -- Volume 78, Issue 4, pp. 377-383
         *    http://arxiv.org/abs/math/0403448v1
         *    Pacific J. Math. 231 (2007), no. 2, 279--291
         */

        static const QSet<const QRegularExpression> journalRegExp{
            /**
             * Structure:
             *  - journal title: letters, space, dot
             *  - optional: spaces
             *  - volume: number
             *  - volume: number
             * Examples:
             *   Journal of Inefficient Algorithms 5 (2003) 35-39
             *   New J. Phys. 10 (2008) 033023
             *   Physics Letters A 297 (2002) 4-8
             *   Appl.Phys. B75 (2002) 655-665
             *   JHEP 0611 (2006) 045
             *  - https://arxiv.org/abs/1105.4915
             *    Astrophys. J. 736 (2011) 7
             *  - https://arxiv.org/abs/astro-ph/0209123
             *    Astrophys.J. 578 (2002) L103-L106
             *  - https://arxiv.org/abs/quant-ph/0611139
             *    Journal of Physics: Conference Series 70 (2007) 012003
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)])\\s*(?<volume>\\d+)\\s+\\((?<year>\\d{4})\\)\\s+(?<pagestart>[0-9A-Z]+)(-{1,2}(?<pageend>[0-9A-Z]+))?$"), QRegularExpression::CaseInsensitiveOption),
            /**
             * Examples:
             *  - https://arxiv.org/abs/1102.5769
             *    The International Journal of Multimedia & Its Applications, 3(1), 2011
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)]),\\s*(?<volume>\\d+)\\((?<number>\\d+)\\),\\s*(?<year>\\d{4})$"), QRegularExpression::CaseInsensitiveOption),
            /**
             * Examples:
             *  Journal of Inefficient Algorithms, Vol. 93, No. 2 (2009), pp. 42-51
             *  Stud. Hist. Phil. Mod. Phys., Vol 33 no 3 (2003), pp. 441-468
             *  - https://arxiv.org/abs/quant-ph/0311028
             *    International Journal of Quantum Information, Vol. 1, No. 4 (2003) 427-441
             *  - https://arxiv.org/abs/1003.3521
             *    Int. J. Geometric Methods in Modern Physics Vol. 8, No. 1 (2011) 155-165
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)]),\\s+Vol\\.?\\s+(?<volume>\\d+)[,]?\\s+No\\.?\\s+(?<number>\\d+)\\s+\\((?<year>\\d{4})\\)[,]?\\s+(pp\\.\\s+)?(?<pagestart>\\d+)(-{1,2}(?<pageend>\\d+))?$"), QRegularExpression::CaseInsensitiveOption),
            /**
             * Examples:
             *   Journal of Inefficient Algorithms, volume 4, number 1, pp. 12-21, 2008
             *  - https://arxiv.org/abs/cs/0601030
             *    Scientometrics, volume 69, number 3, pp. 669-687, 2006
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)]),\\s+volume\\s+(?<volume>\\d+),\\s+number\\s+(?<number>\\d+),\\s+pp\\.\\s+(?<pagestart>\\d+)(-{1,2}(?<pageend>\\d+))?,\\s+(?<year>\\d{4})$"), QRegularExpression::CaseInsensitiveOption),
            /**
             * Examples:
             *  Journal of Inefficient Algorithms 4(1): 101-122, 2010
             *  JHEP0809:131,2008
             *  Phys.Rev.D78:013004,2008
             *  Lect.NotesPhys.690:107-127,2006
             *  Europhys. Letters 70:1-7 (2005)
             *  - https://arxiv.org/abs/quant-ph/0608209
             *    J.Phys.A40:9025-9032,2007
             *  - https://arxiv.org/abs/hep-th/9907001
             *    Phys.Rev.Lett.85:5042-5045,2000
             *  - https://arxiv.org/abs/physics/0606007
             *    Journal of Conflict Resolution 51(1): 58 - 88 (2007)
             *  - https://arxiv.org/abs/arxiv:cs/0609126
             *    Learn.Publ.20:16-22,2007
             *  - https://arxiv.org/abs/cs/9901005
             *    Journal of Artificial Intelligence Research (JAIR), 9:247-293
             *  - https://arxiv.org/abs/hep-ph/0403113
             *    Nucl.Instrum.Meth.A534:250-259,2004
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)])[,]?\\s*(?<volume>\\d+)(\\((?<number>\\d+)\\))?:\\s*(?<pagestart>\\d+)(\\s*-{1,2}\\s*(?<pageend>\\d+))?([, ]\\s*\\(?(?<year>\\d{4})\\)?)?$"), QRegularExpression::CaseInsensitiveOption),
            /**
             * Examples:
             *  Journal of Inefficient Algorithms vol. 31, 4 2000
             *  Phys. Rev. A 71, 032339 (2005)
             *  Phys. Rev. Lett. 91, 027901 (2003)
             *  Phys. Rev. A 78, 013620 (2008)
             *  Phys. Rev. E 62, 1842 (2000)
             *  J. Math. Phys. 49, 032105 (2008)
             *  Phys. Rev. Lett. 91, 217905 (2003).
             *  Physical Review B vol. 66, 161320(R) (2002)
             *  - https://arxiv.org/abs/1003.1050
             *    Phys. Rev. A 82, 012304 (2010)
             *  - https://arxiv.org/abs/quant-ph/0607107
             *    J. Mod. Opt., 54, 2211 (2007)
             *  - https://arxiv.org/abs/quant-ph/0602069
             *    New J. Phys. 8, 58 (2006)
             *  - https://arxiv.org/abs/quant-ph/0610030
             *    Rev. Mod. Phys. 79, 555 (2007)
             *  - https://arxiv.org/abs/1007.2292
             *    J. Phys. A: Math. Theor. 44, 145304 (2011)
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)])\\s+(vol\\.\\s+)?(?<volume>\\d+),\\s+(?<number>\\d+)(\\([A-Z]+\\))?\\s+\\((?<year>\\d{4})\\)[.]?$")),

            /**
             * Examples:
             *  Journal of Inefficient Algorithms, 11(2) (1999) 42-55
             *  Learned Publishing, 20(1) (January 2007) 16-22
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)]),\\s+(?<volume>\\d+)\\((?<number>\\d+)\\)\\s+(\\(([A-Za-z]+\\s+)?(?<year>\\d{4})\\))?\\s+(?<pagestart>\\d+)(-{1,2}(?<pageend>\\d+))?$")),
            /**
             * Examples:
             *  Pacific J. Math. 231 (2007), no. 2, 279–291
             */
            QRegularExpression(QStringLiteral("^(?<journaltitle>[a-z][a-z. &(]+[a-z)])\\s+(?<volume>\\d+)\\s+\\((?<year>\\d{4})\\), no\\.\\s+(?<number>\\d+),\\s+(?<pagestart>\\d+)[^ 0-9]+(?<pageend>\\d+)$"))
        };

        for (const QRegularExpression &regexp : journalRegExp) {
            const QRegularExpressionMatch match = regexp.match(journal);
            if (match.hasMatch()) {
                const QSet<QPair<QString, QString>> keyValuePairs{
                    {Entry::ftJournal, match.captured(QStringLiteral("journaltitle"))},
                    {Entry::ftVolume, match.captured(QStringLiteral("volume"))},
                    {Entry::ftNumber, match.captured(QStringLiteral("number"))},
                    {Entry::ftPages, match.captured(QStringLiteral("pagestart")) + (match.captured(QStringLiteral("pageend")).isEmpty() ? QString() : QChar(0x2013) + match.captured(QStringLiteral("pageend")))},
                    {Entry::ftYear, match.captured(QStringLiteral("year"))}
                };
                for (auto it = keyValuePairs.constBegin(); it != keyValuePairs.constEnd(); ++it) {
                    const QString &key = it->first;
                    const QString &value = it->second;
                    if (!value.isEmpty())
                        entry->insert(key, Value() << QSharedPointer<PlainText>(new PlainText(value)));
                }
                return;
            }
        }

        static const QRegularExpression
        generalJour(QStringLiteral("^[a-z0-9]{,3}[a-z. ]+"), QRegularExpression::CaseInsensitiveOption),
                    generalYear(QStringLiteral("\\b(18|19|20)\\d{2}\\b")),
                    generalPages(QStringLiteral("\\b(?<pagestart>[1-9]\\d{0,2})\\s*[-]+\\s*(?<pageend>[1-9]\\d{0,2})\\b"));

        QRegularExpressionMatch match;

        if ((match = generalJour.match(journal)).hasMatch())
            entry->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(match.captured(0))));
        if ((match = generalYear.match(journal)).hasMatch())
            entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(match.captured(0))));
        if ((match = generalPages.match(journal)).hasMatch()) {
            QString pageText = match.captured(QStringLiteral("pagestart"));
            if (!match.captured(QStringLiteral("pageend")).isEmpty())
                pageText.append(QChar(0x2013) + match.captured(QStringLiteral("pageend")));
            entry->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(pageText)));
        }
    }

    QVector<QSharedPointer<Entry>> parseAtomXML(const QByteArray &xmlData, bool *ok) {
        QVector<QSharedPointer<Entry>> result;

        // Using code generated by Python script 'onlinesearch-parser-generator.py' using
        // information from file 'onlinesearcharxiv-parser.in.cpp'.
        #include "onlinesearch/onlinesearcharxiv-parser.generated.cpp"

        return result;
    }
};


OnlineSearchArXiv::OnlineSearchArXiv(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchArXiv::OnlineSearchArXivPrivate(this))
{
    // nothing
}

OnlineSearchArXiv::~OnlineSearchArXiv()
{
    delete d;
}

#ifdef HAVE_QTWIDGETS
void OnlineSearchArXiv::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchArXiv::downloadDone);

    d->form->saveState();

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchArXiv::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchArXiv::downloadDone);

    refreshBusyProperty();
}

QString OnlineSearchArXiv::label() const
{
#ifdef HAVE_KF
    return i18n("arXiv.org");
#else // HAVE_KF
    //= onlinesearch-arxiv-label
    return QObject::tr("arXiv.org");
#endif // HAVE_KF
}

#ifdef HAVE_QTWIDGETS
OnlineSearchAbstract::Form *OnlineSearchArXiv::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchArXiv::Form(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchArXiv::homepage() const
{
    return QUrl(QStringLiteral("https://arxiv.org/"));
}

#ifdef BUILD_TESTING
QVector<QSharedPointer<Entry> > OnlineSearchArXiv::parseAtomXML(const QByteArray &xmlData, bool *ok)
{
    return d->parseAtomXML(xmlData, ok);
}
#endif // BUILD_TESTING

void OnlineSearchArXiv::downloadDone()
{
    Q_EMIT progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        const QByteArray data{reply->readAll()};
        bool ok = false;
        const QVector<QSharedPointer<Entry>> entries = d->parseAtomXML(data, &ok);
        if (ok) {
            for (const auto &entry : entries)
                publishEntry(entry);
            stopSearch(resultNoError);
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to parse Atom XML data from" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
            stopSearch(resultUnspecifiedError);
        }
    }

    refreshBusyProperty();
}


#include "onlinesearcharxiv.moc"
