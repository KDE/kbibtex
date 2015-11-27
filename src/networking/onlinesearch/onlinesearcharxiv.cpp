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

#include "onlinesearcharxiv.h"

#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QNetworkReply>
#include <QStandardPaths>

#include <KLineEdit>
#include <KLocalizedString>
#include <KMessageBox>
#include <KConfigGroup>

#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

class OnlineSearchArXiv::OnlineSearchQueryFormArXiv : public OnlineSearchQueryFormAbstract
{
private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        lineEditFreeText->setText(configGroup.readEntry(QStringLiteral("freeText"), QString()));
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
    }

public:
    KLineEdit *lineEditFreeText;
    QSpinBox *numResultsField;

    OnlineSearchQueryFormArXiv(QWidget *parent)
            : OnlineSearchQueryFormAbstract(parent), configGroupName(QStringLiteral("Search Engine arXiv.org")) {
        QGridLayout *layout = new QGridLayout(this);
        layout->setMargin(0);

        QLabel *label = new QLabel(i18n("Free text:"), this);
        layout->addWidget(label, 0, 0, 1, 1);
        lineEditFreeText = new KLineEdit(this);
        lineEditFreeText->setClearButtonEnabled(true);
        lineEditFreeText->setFocus(Qt::TabFocusReason);
        layout->addWidget(lineEditFreeText, 0, 1, 1, 1);
        label->setBuddy(lineEditFreeText);
        connect(lineEditFreeText, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

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

    bool readyToStart() const {
        return !lineEditFreeText->text().isEmpty();
    }

    void copyFromEntry(const Entry &entry) {
        lineEditFreeText->setText(authorLastNames(entry).join(QStringLiteral(" ")) + QLatin1Char(' ') + PlainTextValue::text(entry[Entry::ftTitle]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QStringLiteral("freeText"), lineEditFreeText->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        config->sync();
    }
};

class OnlineSearchArXiv::OnlineSearchArXivPrivate
{
private:
    OnlineSearchArXiv *p;

public:
    XSLTransform *xslt;
    OnlineSearchQueryFormArXiv *form;
    const QString arXivQueryBaseUrl;
    int numSteps, curStep;

    OnlineSearchArXivPrivate(OnlineSearchArXiv *parent)
            : p(parent), form(NULL), arXivQueryBaseUrl(QStringLiteral("http://export.arxiv.org/api/query?")),
          numSteps(0), curStep(0)
    {
        const QString xsltFilename = QStringLiteral("kbibtex/arxiv2bibtex.xsl");
        xslt = XSLTransform::createXSLTransform(QStandardPaths::locate(QStandardPaths::GenericDataLocation, xsltFilename));
        if (xslt == NULL)
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not create XSLT transformation for" << xsltFilename;
    }

    ~OnlineSearchArXivPrivate() {
        delete xslt;
    }

    QUrl buildQueryUrl() {
        /// format search terms
        QStringList queryFragments;

        foreach (const QString &queryFragment, p->splitRespectingQuotationMarks(form->lineEditFreeText->text()))
            queryFragments.append(p->encodeURL(queryFragment));
        return QUrl(QString(QStringLiteral("%1search_query=all:\"%3\"&start=0&max_results=%2")).arg(arXivQueryBaseUrl).arg(form->numResultsField->value()).arg(queryFragments.join(QStringLiteral("\"+AND+all:\"")))); ///< join search terms with an AND operation
    }

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        /// format search terms
        QStringList queryFragments;
        for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it)
            foreach (const QString &queryFragment, p->splitRespectingQuotationMarks(it.value()))
                queryFragments.append(p->encodeURL(queryFragment));
        return QUrl(QString(QStringLiteral("%1search_query=all:\"%3\"&start=0&max_results=%2")).arg(arXivQueryBaseUrl).arg(numResults).arg(queryFragments.join(QStringLiteral("\"+AND+all:\"")))); ///< join search terms with an AND operation
    }

    void interpreteJournal(Entry &entry) {
        /**
         * TODO examples
         *  - http://arxiv.org/abs/1111.5338
         *    Eur. Phys. J. H 36, 183-201 (2011)
         *  - http://arxiv.org/abs/1111.5348
         *    IJCSI International Journal of Computer Science Issues, Vol. 8, Issue 3, No. 1, 2011, 224-229
         *  - http://arxiv.org/abs/1110.3379
         *    IJCSI International Journal of Computer Science Issues, Vol. 8, Issue 5, No 3, September 2011 ISSN (Online): 1694-0814
         *  - http://arxiv.org/abs/1102.5769
         *    The International Journal of Multimedia & Its Applications, 3(1), 2011
         *  - http://arxiv.org/abs/1003.3022
         *    American Journal of Physics -- April 2010 -- Volume 78, Issue 4, pp. 377-383
         */
        static const QRegExp
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
         *  - http://arxiv.org/abs/1105.4915
         *    Astrophys. J. 736 (2011) 7
         *  - http://arxiv.org/abs/astro-ph/0209123
         *    Astrophys.J. 578 (2002) L103-L106
         *  - http://arxiv.org/abs/quant-ph/0611139
         *    Journal of Physics: Conference Series 70 (2007) 012003
         * Captures:
         *   1: journal title
         *   2: volume
         *   3: year
         *   4: page start
         *   6: page end
         */
        journalRef1("^([a-z. ]+[a-z.])\\s*(\\d+)\\s+\\((\\d{4})\\)\\s+([0-9A-Z]+)(-([0-9A-Z]+))?$", Qt::CaseInsensitive),

                    /**
                     * Examples:
                     *  Journal of Inefficient Algorithms, Vol. 93, No. 2 (2009), pp. 42-51
                     *  Stud. Hist. Phil. Mod. Phys., Vol 33 no 3 (2003), pp. 441-468
                     *  - http://arxiv.org/abs/quant-ph/0311028
                     *    International Journal of Quantum Information, Vol. 1, No. 4 (2003) 427-441
                     *  - http://arxiv.org/abs/1003.3521
                     *    Int. J. Geometric Methods in Modern Physics Vol. 8, No. 1 (2011) 155-165
                     * Captures:
                     *   1: journal title
                     *   2: volume
                     *   3: number
                     *   4: year
                     *   5: page start
                     *   7: page end
                     */
                    journalRef2("^([a-zA-Z. ]+[a-zA-Z.]),\\s+Vol\\.?\\s+(\\d+)[,]?\\s+No\\.?\\s+(\\d+)\\s+\\((\\d{4})\\)[,]?\\s+(pp\\.\\s+)?(\\d+)(-(\\d+))?$", Qt::CaseInsensitive),

                    /**
                     * Examples:
                     *   Journal of Inefficient Algorithms, volume 4, number 1, pp. 12-21, 2008
                     *  - http://arxiv.org/abs/cs/0601030
                     *    Scientometrics, volume 69, number 3, pp. 669-687, 2006
                     * Captures:
                     *   1: journal title
                     *   2: volume
                     *   3: number
                     *   4: page start
                     *   6: page end
                     *   7: year
                     */
                    journalRef3("^([a-zA-Z. ]+),\\s+volume\\s+(\\d+),\\s+number\\s+(\\d+),\\s+pp\\.\\s+(\\d+)(-(\\d+))?,\\s+(\\d{4})$", Qt::CaseInsensitive),

                    /**
                     * Examples:
                     *  Journal of Inefficient Algorithms 4(1): 101-122, 2010
                     *  JHEP0809:131,2008
                     *  Phys.Rev.D78:013004,2008
                     *  Lect.NotesPhys.690:107-127,2006
                     *  Europhys. Letters 70:1-7 (2005)
                     *  - http://arxiv.org/abs/quant-ph/0608209
                     *    J.Phys.A40:9025-9032,2007
                     *  - http://arxiv.org/abs/hep-th/9907001
                     *    Phys.Rev.Lett.85:5042-5045,2000
                     *  - http://arxiv.org/abs/physics/0606007
                     *    Journal of Conflict Resolution 51(1): 58 - 88 (2007)
                     *  - http://arxiv.org/abs/arxiv:cs/0609126
                     *    Learn.Publ.20:16-22,2007
                     *  - http://arxiv.org/abs/cs/9901005
                     *    Journal of Artificial Intelligence Research (JAIR), 9:247-293
                     *  - http://arxiv.org/abs/hep-ph/0403113
                     *    Nucl.Instrum.Meth.A534:250-259,2004
                     * Captures:
                     *   1: journal title
                     *   2: volume
                     *   4: number
                     *   5: page start
                     *   7: page end
                     *   9 or 10: year
                     */
                    journalRef4("^([a-z. ()]+)[,]?\\s*(\\d+)(\\((\\d+)\\))?:\\s*(\\d+)(\\s*-\\s*(\\d+))?(,\\s*(\\d{4})|\\s+\\((\\d{4})\\))?$", Qt::CaseInsensitive),

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
                     *  - http://arxiv.org/abs/1003.1050
                     *    Phys. Rev. A 82, 012304 (2010)
                     *  - http://arxiv.org/abs/quant-ph/0607107
                     *    J. Mod. Opt., 54, 2211 (2007)
                     *  - http://arxiv.org/abs/quant-ph/0602069
                     *    New J. Phys. 8, 58 (2006)
                     *  - http://arxiv.org/abs/quant-ph/0610030
                     *    Rev. Mod. Phys. 79, 555 (2007)
                     *  - http://arxiv.org/abs/1007.2292
                     *    J. Phys. A: Math. Theor. 44, 145304 (2011)
                     * Captures:
                     *   1: journal title
                     *   3: volume
                     *   4: page start
                     *   6: year
                     */
                    journalRef5("^([a-zA-Z. ]+)\\s+(vol\\.\\s+)?(\\d+),\\s+(\\d+)(\\([A-Z]+\\))?\\s+\\((\\d{4})\\)[.]?$", Qt::CaseInsensitive),

                    /**
                     * Examples:
                     *  Journal of Inefficient Algorithms, 11(2) (1999) 42-55
                     *  Learned Publishing, 20(1) (January 2007) 16-22
                     * Captures:
                     *   1: journal title
                     *   2: volume
                     *   3: number
                     *   4: year
                     *   5: page start
                     *   7: page end
                     */
                    journalRef6("^([a-zA-Z. ]+),\\s+(\\d+)\\((\\d+)\\)\\s+(\\(([A-Za-z]+\\s+)?(\\d{4})\\))?\\s+(\\d+)(-(\\d+))?$", Qt::CaseInsensitive),
                    generalJour("^[a-z0-9]{,3}[a-z. ]+", Qt::CaseInsensitive), generalYear("\\b(18|19|20)\\d{2}\\b"), generalPages("\\b([1-9]\\d{0,2})\\s*[-]+\\s*([1-9]\\d{0,2})\\b");
        QString journalText = PlainTextValue::text(entry.value(Entry::ftJournal));

        /// nothing to do on empty journal text
        if (journalText.isEmpty()) return;
        else entry.remove(Entry::ftJournal);

        if (journalRef1.indexIn(journalText, Qt::CaseInsensitive) >= 0) {
            /**
             * Captures:
             *   1: journal title
             *   2: volume
             *   3: year
             *   4: page start
             *   6: page end
             */

            QString text;
            if (!(text = journalRef1.cap(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = journalRef1.cap(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = journalRef1.cap(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }
            if (!(text = journalRef1.cap(4)).isEmpty()) {
                const QString endPage = journalRef1.cap(6);
                if (endPage.isEmpty()) {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(text)));
                    entry.insert(Entry::ftPages, v);
                } else {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(QString(QStringLiteral("%1--%2")).arg(text, endPage))));
                    entry.insert(Entry::ftPages, v);
                }
            }
        } else if (journalRef2.indexIn(journalText, Qt::CaseInsensitive) >= 0) {
            /**
             * Captures:
             *   1: journal title
             *   2: volume
             *   3: number
             *   4: year
             *   5: page start
             *   7: page end
             */

            QString text;
            if (!(text = journalRef2.cap(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = journalRef2.cap(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = journalRef2.cap(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftNumber, v);
            }
            if (!(text = journalRef2.cap(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }
            if (!(text = journalRef2.cap(5)).isEmpty()) {
                const QString endPage = journalRef2.cap(7);
                if (endPage.isEmpty()) {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(text)));
                    entry.insert(Entry::ftPages, v);
                } else {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(QString(QStringLiteral("%1%3%2")).arg(text, endPage, QChar(0x2013)))));
                    entry.insert(Entry::ftPages, v);
                }
            }
        } else if (journalRef3.indexIn(journalText, Qt::CaseInsensitive) >= 0) {
            /**
             * Captures:
             *   1: journal title
             *   2: volume
             *   4: number
             *   5: page start
             *   7: page end
             *   9 or 10: year
             */

            QString text;
            if (!(text = journalRef3.cap(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = journalRef3.cap(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = journalRef3.cap(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftNumber, v);
            }
            if (!(text = journalRef3.cap(9)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
                if (!(text = journalRef3.cap(10)).isEmpty()) {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(text)));
                    entry.insert(Entry::ftYear, v);
                }
                if (!(text = journalRef2.cap(5)).isEmpty()) {
                    const QString endPage = journalRef2.cap(7);
                    if (endPage.isEmpty()) {
                        Value v;
                        v.append(QSharedPointer<PlainText>(new PlainText(text)));
                        entry.insert(Entry::ftPages, v);
                    } else {
                        Value v;
                        v.append(QSharedPointer<PlainText>(new PlainText(QString(QStringLiteral("%1%3%2")).arg(text, endPage, QChar(0x2013)))));
                        entry.insert(Entry::ftPages, v);
                    }
                }
            }

        } else if (journalRef4.indexIn(journalText, Qt::CaseInsensitive) >= 0) {
            /**
             * Captures:
             *   1: journal title
             *   2: volume
             *   4: number
             *   5: page start
             *   7: page end
             *   9 or 10: year
             */

            QString text;
            if (!(text = journalRef4.cap(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = journalRef4.cap(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = journalRef4.cap(5)).isEmpty()) {
                const QString endPage = journalRef4.cap(7);
                if (endPage.isEmpty()) {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(text)));
                    entry.insert(Entry::ftPages, v);
                } else {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(text.append(QChar(0x2013)).append(endPage))));
                    entry.insert(Entry::ftPages, v);
                }
            }
            if (!(text = journalRef4.cap(9)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }

        } else if (journalRef5.indexIn(journalText, Qt::CaseInsensitive) >= 0) {
            /** Captures:
              *   1: journal title
              *   3: volume
              *   4: page start
              *   6: year
              */

            QString text;
            if (!(text = journalRef5.cap(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = journalRef5.cap(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = journalRef5.cap(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftPages, v);
            }
            if (!(text = journalRef5.cap(6)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }

        } else if (journalRef6.indexIn(journalText, Qt::CaseInsensitive) >= 0) {
            /** Captures:
              *   1: journal title
              *   2: volume
              *   3: number
              *   4: year
              *   5: page start
              *   7: page end
              */

            QString text;
            if (!(text = journalRef6.cap(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = journalRef6.cap(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = journalRef6.cap(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftNumber, v);
            }
            if (!(text = journalRef6.cap(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }
            if (!(text = journalRef6.cap(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftPages, v);
            }
            if (!(text = journalRef6.cap(5)).isEmpty()) {
                const QString endPage = journalRef6.cap(7);
                if (endPage.isEmpty()) {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(text)));
                    entry.insert(Entry::ftPages, v);
                } else {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(QString(QStringLiteral("%1%3%2")).arg(text, endPage, QChar(0x2013)))));
                    entry.insert(Entry::ftPages, v);
                }
            }
        } else {
            if (generalJour.indexIn(journalText) >= 0) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(generalJour.cap(0))));
                entry.insert(Entry::ftPages, v);
            }
            if (generalYear.indexIn(journalText) >= 0) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(generalYear.cap(0))));
                entry.insert(Entry::ftYear, v);
            }
            if (generalPages.indexIn(journalText) >= 0) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(generalPages.cap(0))));
                entry.insert(Entry::ftPages, v);
            }
        }
    }
};

OnlineSearchArXiv::OnlineSearchArXiv(QWidget *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchArXiv::OnlineSearchArXivPrivate(this))
{
    // nothing
}

OnlineSearchArXiv::~OnlineSearchArXiv()
{
    delete d;
}

void OnlineSearchArXiv::startSearch()
{
    if (d->xslt == NULL) {
        /// Don't allow searches if xslt is not defined
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot allow searching" << label() << "if XSL Transformation not available";
        delayedStoppedSearch(resultUnspecifiedError);
        return;
    }

    d->curStep = 0;
    d->numSteps = 1;
    m_hasBeenCanceled = false;

    QNetworkRequest request(d->buildQueryUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, d->numSteps);

    d->form->saveState();
}

void OnlineSearchArXiv::startSearch(const QMap<QString, QString> &query, int numResults)
{
    if (d->xslt == NULL) {
        /// Don't allow searches if xslt is not defined
        qCWarning(LOG_KBIBTEX_NETWORKING) << "Cannot allow searching" << label() << "if XSL Transformation not available";
        delayedStoppedSearch(resultUnspecifiedError);
        return;
    }

    d->curStep = 0;
    d->numSteps = 1;
    m_hasBeenCanceled = false;

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::self()->get(request);
    InternalNetworkAccessManager::self()->setNetworkReplyTimeout(reply);
    connect(reply, SIGNAL(finished()), this, SLOT(downloadDone()));

    emit progress(0, d->numSteps);
}

QString OnlineSearchArXiv::label() const
{
    return i18n("arXiv.org");
}

QString OnlineSearchArXiv::favIconUrl() const
{
    return QStringLiteral("http://arxiv.org/favicon.ico");
}

OnlineSearchQueryFormAbstract *OnlineSearchArXiv::customWidget(QWidget *parent)
{
    return (d->form = new OnlineSearchArXiv::OnlineSearchQueryFormArXiv(parent));
}

QUrl OnlineSearchArXiv::homepage() const
{
    return QUrl(QStringLiteral("http://arxiv.org/"));
}

void OnlineSearchArXiv::cancel()
{
    OnlineSearchAbstract::cancel();
}

void OnlineSearchArXiv::downloadDone()
{
    emit progress(++d->curStep, d->numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString result = QString::fromUtf8(reply->readAll().data());
        result = result.remove(QStringLiteral("xmlns=\"http://www.w3.org/2005/Atom\"")); // FIXME fix arxiv2bibtex.xsl to handle namespace

        /// use XSL transformation to get BibTeX document from XML result
        QString bibTeXcode = d->xslt->transform(result).remove(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));

        FileImporterBibTeX importer;
        File *bibtexFile = importer.fromString(bibTeXcode);

        bool hasEntries = false;
        if (bibtexFile != NULL) {
            for (File::ConstIterator it = bibtexFile->constBegin(); it != bibtexFile->constEnd(); ++it) {
                QSharedPointer<Entry> entry = (*it).dynamicCast<Entry>();
                hasEntries |= publishEntry(entry);
            }

            emit stoppedSearch(resultNoError);
            emit progress(d->numSteps, d->numSteps);

            delete bibtexFile;
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "No valid BibTeX file results returned on request on" << reply->url().toString();
            emit stoppedSearch(resultUnspecifiedError);
        }
    } else
        qCWarning(LOG_KBIBTEX_NETWORKING) << "url was" << reply->url().toString();
}

void OnlineSearchArXiv::sanitizeEntry(QSharedPointer<Entry> entry)
{
    OnlineSearchAbstract::sanitizeEntry(entry);

    d->interpreteJournal(*entry);
}
