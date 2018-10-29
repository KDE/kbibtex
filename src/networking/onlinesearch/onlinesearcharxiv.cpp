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

#include "onlinesearcharxiv.h"

#include <QNetworkReply>
#ifdef HAVE_QTWIDGETS
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QTextStream>
#endif // HAVE_QTWIDGETS

#ifdef HAVE_QTWIDGETS
#include <KLineEdit>
#include <KMessageBox>
#endif // HAVE_QTWIDGETS

#ifdef HAVE_KF5
#include <KConfigGroup>
#include <KLocalizedString>
#endif // HAVE_KF5

#include "fileimporterbibtex.h"
#include "xsltransform.h"
#include "encoderxml.h"
#include "internalnetworkaccessmanager.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
class OnlineSearchArXiv::OnlineSearchQueryFormArXiv : public OnlineSearchQueryFormAbstract
{
    Q_OBJECT

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
        connect(lineEditFreeText, &KLineEdit::returnPressed, this, &OnlineSearchQueryFormArXiv::returnPressed);

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
        lineEditFreeText->setText(authorLastNames(entry).join(QStringLiteral(" ")) + QLatin1Char(' ') + PlainTextValue::text(entry[Entry::ftTitle]));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(QStringLiteral("freeText"), lineEditFreeText->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        config->sync();
    }
};
#endif // HAVE_QTWIDGETS

class OnlineSearchArXiv::OnlineSearchArXivPrivate
{
private:
    static const QString xsltFilenameBase;

public:
    const XSLTransform xslt;
#ifdef HAVE_QTWIDGETS
    OnlineSearchQueryFormArXiv *form;
#endif // HAVE_QTWIDGETS
    const QString arXivQueryBaseUrl;

    OnlineSearchArXivPrivate(OnlineSearchArXiv *)
            : xslt(XSLTransform::locateXSLTfile(xsltFilenameBase)),
#ifdef HAVE_QTWIDGETS
          form(nullptr),
#endif // HAVE_QTWIDGETS
          arXivQueryBaseUrl(QStringLiteral("https://export.arxiv.org/api/query?"))
    {
        if (!xslt.isValid())
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to initialize XSL transformation based on file '" << xsltFilenameBase << "'";
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

    QUrl buildQueryUrl(const QMap<QString, QString> &query, int numResults) {
        /// format search terms
        QStringList queryFragments;
        for (QMap<QString, QString>::ConstIterator it = query.constBegin(); it != query.constEnd(); ++it) {
            const auto respectingQuotationMarks = OnlineSearchAbstract::splitRespectingQuotationMarks(it.value());
            for (const auto &queryFragment : respectingQuotationMarks)
                queryFragments.append(OnlineSearchAbstract::encodeURL(queryFragment));
        }
        return QUrl(QString(QStringLiteral("%1search_query=all:\"%3\"&start=0&max_results=%2")).arg(arXivQueryBaseUrl).arg(numResults).arg(queryFragments.join(QStringLiteral("\"+AND+all:\"")))); ///< join search terms with an AND operation
    }

    void interpreteJournal(Entry &entry) {
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
         */
        static const QRegularExpression
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
         * Captures:
         *   1: journal title
         *   2: volume
         *   3: year
         *   4: page start
         *   6: page end
         */
        journalRef1(QStringLiteral("^([a-z. ]+[a-z.])\\s*(\\d+)\\s+\\((\\d{4})\\)\\s+([0-9A-Z]+)(-([0-9A-Z]+))?$"), QRegularExpression::CaseInsensitiveOption),
        /**
         * Examples:
         *  Journal of Inefficient Algorithms, Vol. 93, No. 2 (2009), pp. 42-51
         *  Stud. Hist. Phil. Mod. Phys., Vol 33 no 3 (2003), pp. 441-468
         *  - https://arxiv.org/abs/quant-ph/0311028
         *    International Journal of Quantum Information, Vol. 1, No. 4 (2003) 427-441
         *  - https://arxiv.org/abs/1003.3521
         *    Int. J. Geometric Methods in Modern Physics Vol. 8, No. 1 (2011) 155-165
         * Captures:
         *   1: journal title
         *   2: volume
         *   3: number
         *   4: year
         *   5: page start
         *   7: page end
         */
        journalRef2(QStringLiteral("^([a-zA-Z. ]+[a-zA-Z.]),\\s+Vol\\.?\\s+(\\d+)[,]?\\s+No\\.?\\s+(\\d+)\\s+\\((\\d{4})\\)[,]?\\s+(pp\\.\\s+)?(\\d+)(-(\\d+))?$"), QRegularExpression::CaseInsensitiveOption),
        /**
         * Examples:
         *   Journal of Inefficient Algorithms, volume 4, number 1, pp. 12-21, 2008
         *  - https://arxiv.org/abs/cs/0601030
         *    Scientometrics, volume 69, number 3, pp. 669-687, 2006
         * Captures:
         *   1: journal title
         *   2: volume
         *   3: number
         *   4: page start
         *   6: page end
         *   7: year
         */
        journalRef3(QStringLiteral("^([a-zA-Z. ]+),\\s+volume\\s+(\\d+),\\s+number\\s+(\\d+),\\s+pp\\.\\s+(\\d+)(-(\\d+))?,\\s+(\\d{4})$"), QRegularExpression::CaseInsensitiveOption),
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
         * Captures:
         *   1: journal title
         *   2: volume
         *   4: number
         *   5: page start
         *   7: page end
         *   9 or 10: year
         */
        journalRef4(QStringLiteral("^([a-z. ()]+)[,]?\\s*(\\d+)(\\((\\d+)\\))?:\\s*(\\d+)(\\s*-\\s*(\\d+))?(,\\s*(\\d{4})|\\s+\\((\\d{4})\\))?$"), QRegularExpression::CaseInsensitiveOption),
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
         * Captures:
         *   1: journal title
         *   3: volume
         *   4: page start
         *   6: year
         */
        journalRef5(QStringLiteral("^([a-zA-Z. ]+)\\s+(vol\\.\\s+)?(\\d+),\\s+(\\d+)(\\([A-Z]+\\))?\\s+\\((\\d{4})\\)[.]?$")),
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
        journalRef6(QStringLiteral("^([a-zA-Z. ]+),\\s+(\\d+)\\((\\d+)\\)\\s+(\\(([A-Za-z]+\\s+)?(\\d{4})\\))?\\s+(\\d+)(-(\\d+))?$")),
        /**
         * Examples:
         *  Pacific J. Math. 231 (2007), no. 2, 279–291
         * Captures:
         *   1: journal title
         *   2: volume
         *   3: year
         *   4: number
         *   5: page start
         *   6: page end
         */
        journalRef7(QStringLiteral("^([a-zA-Z. ]+[a-zA-Z.])\\s+(\\d+)\\s+\\((\\d+)\\), no\\.\\s+(\\d+),\\s+(\\d)[^ ]+(\\d+)$")),
        generalJour(QStringLiteral("^[a-z0-9]{,3}[a-z. ]+"), QRegularExpression::CaseInsensitiveOption),
        generalYear(QStringLiteral("\\b(18|19|20)\\d{2}\\b")),
        generalPages(QStringLiteral("\\b([1-9]\\d{0,2})\\s*[-]+\\s*([1-9]\\d{0,2})\\b"));
        const QString journalText = PlainTextValue::text(entry.value(Entry::ftJournal));

        /// nothing to do on empty journal text
        if (journalText.isEmpty()) return;
        else entry.remove(Entry::ftJournal);

        QRegularExpressionMatch match;
        if ((match = journalRef1.match(journalText)).hasMatch()) {
            /**
             * Captures:
             *   1: journal title
             *   2: volume
             *   3: year
             *   4: page start
             *   6: page end
             */

            QString text;
            if (!(text = match.captured(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = match.captured(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = match.captured(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }
            if (!(text = match.captured(4)).isEmpty()) {
                const QString endPage = match.captured(6);
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
        } else if ((match = journalRef2.match(journalText)).hasMatch()) {
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
            if (!(text = match.captured(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = match.captured(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = match.captured(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftNumber, v);
            }
            if (!(text = match.captured(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }
            if (!(text = match.captured(5)).isEmpty()) {
                const QString endPage = match.captured(7);
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
        } else if ((match = journalRef3.match(journalText)).hasMatch()) {
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
            if (!(text = match.captured(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = match.captured(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = match.captured(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftNumber, v);
            }
            if (!(text = match.captured(9)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
                if (!(text = match.captured(10)).isEmpty()) {
                    Value v;
                    v.append(QSharedPointer<PlainText>(new PlainText(text)));
                    entry.insert(Entry::ftYear, v);
                }
                if (!(text = match.captured(5)).isEmpty()) {
                    const QString endPage = match.captured(7);
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

        } else if ((match = journalRef4.match(journalText)).hasMatch()) {
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
            if (!(text = match.captured(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = match.captured(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = match.captured(5)).isEmpty()) {
                const QString endPage = match.captured(7);
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
            if (!(text = match.captured(9)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }

        } else if ((match = journalRef5.match(journalText)).hasMatch()) {
            /** Captures:
              *   1: journal title
              *   3: volume
              *   4: page start
              *   6: year
              */

            QString text;
            if (!(text = match.captured(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = match.captured(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = match.captured(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftPages, v);
            }
            if (!(text = match.captured(6)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }

        } else if ((match = journalRef6.match(journalText)).hasMatch()) {
            /** Captures:
              *   1: journal title
              *   2: volume
              *   3: number
              *   4: year
              *   5: page start
              *   7: page end
              */

            QString text;
            if (!(text = match.captured(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = match.captured(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = match.captured(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftNumber, v);
            }
            if (!(text = match.captured(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }
            if (!(text = match.captured(5)).isEmpty()) {
                const QString endPage = match.captured(7);
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

        } else if ((match = journalRef7.match(journalText)).hasMatch()) {
            /** Captures:
             *   1: journal title
             *   2: volume
             *   3: year
             *   4: number
             *   5: page start
             *   6: page end
             */

            QString text;
            if (!(text = match.captured(1)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftJournal, v);
            }
            if (!(text = match.captured(2)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftVolume, v);
            }
            if (!(text = match.captured(3)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftYear, v);
            }
            if (!(text = match.captured(4)).isEmpty()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(text)));
                entry.insert(Entry::ftNumber, v);
            }
            if (!(text = match.captured(5)).isEmpty()) {
                const QString endPage = match.captured(6);
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
            if ((match = generalJour.match(journalText)).hasMatch()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(match.captured(0))));
                entry.insert(Entry::ftPages, v);
            }
            if ((match = generalYear.match(journalText)).hasMatch()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(match.captured(0))));
                entry.insert(Entry::ftYear, v);
            }
            if ((match = generalPages.match(journalText)).hasMatch()) {
                Value v;
                v.append(QSharedPointer<PlainText>(new PlainText(match.captured(0))));
                entry.insert(Entry::ftPages, v);
            }
        }
    }
};

const QString OnlineSearchArXiv::OnlineSearchArXivPrivate::xsltFilenameBase = QStringLiteral("arxiv2bibtex.xsl");


OnlineSearchArXiv::OnlineSearchArXiv(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchArXiv::OnlineSearchArXivPrivate(this))
{
    /// nothing
}

OnlineSearchArXiv::~OnlineSearchArXiv()
{
    delete d;
}

#ifdef HAVE_QTWIDGETS
void OnlineSearchArXiv::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl());
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchArXiv::downloadDone);

    d->form->saveState();

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchArXiv::startSearch(const QMap<QString, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;
    emit progress(curStep = 0, numSteps = 1);

    QNetworkRequest request(d->buildQueryUrl(query, numResults));
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchArXiv::downloadDone);

    refreshBusyProperty();
}

QString OnlineSearchArXiv::label() const
{
    return i18n("arXiv.org");
}

QString OnlineSearchArXiv::favIconUrl() const
{
    return QStringLiteral("https://arxiv.org/favicon.ico");
}

#ifdef HAVE_QTWIDGETS
OnlineSearchQueryFormAbstract *OnlineSearchArXiv::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new OnlineSearchArXiv::OnlineSearchQueryFormArXiv(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchArXiv::homepage() const
{
    return QUrl(QStringLiteral("https://arxiv.org/"));
}

void OnlineSearchArXiv::downloadDone()
{
    emit progress(++curStep, numSteps);

    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());

    if (handleErrors(reply)) {
        /// ensure proper treatment of UTF-8 characters
        QString result = QString::fromUtf8(reply->readAll().constData());
        result = result.remove(QStringLiteral("xmlns=\"http://www.w3.org/2005/Atom\"")); // FIXME fix arxiv2bibtex.xsl to handle namespace

        /// use XSL transformation to get BibTeX document from XML result
        const QString bibTeXcode = EncoderXML::instance().decode(d->xslt.transform(result));
        if (bibTeXcode.isEmpty()) {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "XSL tranformation failed for data from " << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
            stopSearch(resultInvalidArguments);
        } else {
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
        }
    }

    refreshBusyProperty();
}

void OnlineSearchArXiv::sanitizeEntry(QSharedPointer<Entry> entry)
{
    OnlineSearchAbstract::sanitizeEntry(entry);

    d->interpreteJournal(*entry);
}

#include "onlinesearcharxiv.moc"
