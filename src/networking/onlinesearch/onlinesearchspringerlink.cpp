/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "onlinesearchspringerlink.h"

#ifdef HAVE_QTWIDGETS
#include <QFormLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#endif // HAVE_QTWIDGETS
#include <QRegularExpression>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QXmlStreamReader>

#ifdef HAVE_KF
#include <KLocalizedString>
#include <KConfigGroup>
#else // HAVE_KF
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF

#include <KBibTeX>
#include <Encoder>
#include <FileImporterBibTeX>
#include "internalnetworkaccessmanager.h"
#include "onlinesearchabstract_p.h"
#include "logging_networking.h"

#ifdef HAVE_QTWIDGETS
/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class OnlineSearchSpringerLink::Form : public OnlineSearchAbstract::Form
{
    Q_OBJECT

private:
    QString configGroupName;

    void loadState() {
        KConfigGroup configGroup(d->config, configGroupName);
        lineEditFreeText->setText(configGroup.readEntry(QStringLiteral("free"), QString()));
        lineEditTitle->setText(configGroup.readEntry(QStringLiteral("title"), QString()));
        lineEditBookTitle->setText(configGroup.readEntry(QStringLiteral("bookTitle"), QString()));
        lineEditAuthorEditor->setText(configGroup.readEntry(QStringLiteral("authorEditor"), QString()));
        lineEditYear->setText(configGroup.readEntry(QStringLiteral("year"), QString()));
        numResultsField->setValue(configGroup.readEntry(QStringLiteral("numResults"), 10));
    }

public:
    QLineEdit *lineEditFreeText, *lineEditTitle, *lineEditBookTitle, *lineEditAuthorEditor, *lineEditYear;
    QSpinBox *numResultsField;

    Form(QWidget *parent)
            : OnlineSearchAbstract::Form(parent), configGroupName(QStringLiteral("Search Engine SpringerLink")) {
        QFormLayout *layout = new QFormLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        lineEditFreeText = new QLineEdit(this);
        lineEditFreeText->setClearButtonEnabled(true);
        QLabel *label = new QLabel(i18n("Free Text:"), this);
        label->setBuddy(lineEditFreeText);
        layout->addRow(label, lineEditFreeText);
        connect(lineEditFreeText, &QLineEdit::returnPressed, this, &OnlineSearchSpringerLink::Form::returnPressed);

        lineEditTitle = new QLineEdit(this);
        lineEditTitle->setClearButtonEnabled(true);
        label = new QLabel(i18n("Title:"), this);
        label->setBuddy(lineEditTitle);
        layout->addRow(label, lineEditTitle);
        connect(lineEditTitle, &QLineEdit::returnPressed, this, &OnlineSearchSpringerLink::Form::returnPressed);

        lineEditBookTitle = new QLineEdit(this);
        lineEditBookTitle->setClearButtonEnabled(true);
        label = new QLabel(i18n("Book/Journal title:"), this);
        label->setBuddy(lineEditBookTitle);
        layout->addRow(label, lineEditBookTitle);
        connect(lineEditBookTitle, &QLineEdit::returnPressed, this, &OnlineSearchSpringerLink::Form::returnPressed);

        lineEditAuthorEditor = new QLineEdit(this);
        lineEditAuthorEditor->setClearButtonEnabled(true);
        label = new QLabel(i18n("Author or Editor:"), this);
        label->setBuddy(lineEditAuthorEditor);
        layout->addRow(label, lineEditAuthorEditor);
        connect(lineEditAuthorEditor, &QLineEdit::returnPressed, this, &OnlineSearchSpringerLink::Form::returnPressed);

        lineEditYear = new QLineEdit(this);
        lineEditYear->setClearButtonEnabled(true);
        label = new QLabel(i18n("Year:"), this);
        label->setBuddy(lineEditYear);
        layout->addRow(label, lineEditYear);
        connect(lineEditYear, &QLineEdit::returnPressed, this, &OnlineSearchSpringerLink::Form::returnPressed);

        numResultsField = new QSpinBox(this);
        label = new QLabel(i18n("Number of Results:"), this);
        label->setBuddy(numResultsField);
        layout->addRow(label, numResultsField);
        numResultsField->setMinimum(3);
        numResultsField->setMaximum(25);

        lineEditFreeText->setFocus(Qt::TabFocusReason);

        loadState();
    }

    bool readyToStart() const override {
        return !(lineEditFreeText->text().isEmpty() && lineEditTitle->text().isEmpty() && lineEditBookTitle->text().isEmpty() && lineEditAuthorEditor->text().isEmpty());
    }

    void copyFromEntry(const Entry &entry) override {
        lineEditTitle->setText(PlainTextValue::text(entry[Entry::ftTitle]));
        QString bookTitle = PlainTextValue::text(entry[Entry::ftBookTitle]);
        if (bookTitle.isEmpty())
            bookTitle = PlainTextValue::text(entry[Entry::ftJournal]);
        lineEditBookTitle->setText(bookTitle);
        lineEditAuthorEditor->setText(d->authorLastNames(entry).join(QStringLiteral(" ")));
    }

    void saveState() {
        KConfigGroup configGroup(d->config, configGroupName);
        configGroup.writeEntry(QStringLiteral("free"), lineEditFreeText->text());
        configGroup.writeEntry(QStringLiteral("title"), lineEditTitle->text());
        configGroup.writeEntry(QStringLiteral("bookTitle"), lineEditBookTitle->text());
        configGroup.writeEntry(QStringLiteral("authorEditor"), lineEditAuthorEditor->text());
        configGroup.writeEntry(QStringLiteral("year"), lineEditYear->text());
        configGroup.writeEntry(QStringLiteral("numResults"), numResultsField->value());
        d->config->sync();
    }
};
#endif // HAVE_QTWIDGETS

class OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate
{
private:
    static const QString springerLinkQueryBaseUrl;

public:
    static const QString springerMetadataKey;
#ifdef HAVE_QTWIDGETS
    OnlineSearchSpringerLink::Form *form;
#endif // HAVE_QTWIDGETS

    OnlineSearchSpringerLinkPrivate(OnlineSearchSpringerLink *)
#ifdef HAVE_QTWIDGETS
            : form(nullptr)
#endif // HAVE_QTWIDGETS
    {
        // nothing
    }

#ifdef HAVE_QTWIDGETS
    QUrl buildQueryUrl() {
        if (form == nullptr) return QUrl();

        QUrl queryUrl{QUrl::fromUserInput(springerLinkQueryBaseUrl)};

        QString queryString = form->lineEditFreeText->text();

        const QStringList titleChunks = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditTitle->text());
        for (const QString &titleChunk : titleChunks) {
            /// Query parameter 'title:' is a Premium Plan feature at Springer Nature.
            // queryString += QString(QStringLiteral(" title:%1")).arg(Encoder::instance().convertToPlainAscii(titleChunk));
            /// Thus, searching just for the title's text without telling API that this is a title
            queryString += QString(QStringLiteral(" %1")).arg(Encoder::instance().convertToPlainAscii(titleChunk));
        }

        const QStringList bookTitleChunks = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditBookTitle->text());
        for (const QString &titleChunk : bookTitleChunks) {
            /// Query parameters 'journal:' and 'book:' are a Premium Plan features at Springer Nature.
            // queryString += QString(QStringLiteral(" ( journal:%1 OR book:%1 )")).arg(Encoder::instance().convertToPlainAscii(titleChunk));
            /// Thus, searching just for the book title's text without telling API that this is a journal or book
            queryString += QString(QStringLiteral(" %1")).arg(Encoder::instance().convertToPlainAscii(titleChunk));
        }

        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(form->lineEditAuthorEditor->text());
        for (const QString &author : authors) {
            /// Query parameter 'name:' is a Premium Plan feature at Springer Nature.
            // queryString += QString(QStringLiteral(" name:%1")).arg(Encoder::instance().convertToPlainAscii(author));
            /// Thus, searching just for the name's text without telling API that this is a name
            queryString += QString(QStringLiteral(" %1")).arg(Encoder::instance().convertToPlainAscii(author));
        }

        const QString year = form->lineEditYear->text();
        if (!year.isEmpty()) {
            /// Query parameter 'year:' is a Premium Plan feature at Springer Nature.
            // queryString += QString(QStringLiteral(" year:%1")).arg(year);
            /// Thus, searching just for the year's text without telling API that this is a year
            queryString += QString(QStringLiteral(" %1")).arg(year);
        }

        const int numResults = form->numResultsField->value();

        queryString = queryString.simplified();
        QUrlQuery query(queryUrl);
        query.addQueryItem(QStringLiteral("q"), queryString);
        query.addQueryItem(QStringLiteral("p"), QString::number(qMin(1, qMax(25, numResults)))); //< Enforce sane number of request (max 25 for free Basic plan)
        query.addQueryItem(QStringLiteral("api_key"), springerMetadataKey);
        queryUrl.setQuery(query);

        return queryUrl;
    }
#endif // HAVE_QTWIDGETS

    QUrl buildQueryUrl(const QMap<QueryKey, QString> &query, int numResults) {
        QUrl queryUrl{QUrl::fromUserInput(springerLinkQueryBaseUrl)};
        QString queryString = query[QueryKey::FreeText];

        const QStringList titleChunks = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Title]);
        for (const QString &titleChunk : titleChunks) {
            /// Query parameter 'title:' is a Premium Plan feature at Springer Nature.
            // queryString += QString(QStringLiteral(" title:%1")).arg(Encoder::instance().convertToPlainAscii(titleChunk));
            /// Thus, searching just for the title's text without telling API that this is a title
            queryString += QString(QStringLiteral(" %1")).arg(Encoder::instance().convertToPlainAscii(titleChunk));
        }

        const QStringList authors = OnlineSearchAbstract::splitRespectingQuotationMarks(query[QueryKey::Author]);
        for (const QString &author : authors) {
            /// Query parameter 'name:' is a Premium Plan feature at Springer Nature.
            // queryString += QString(QStringLiteral(" name:%1")).arg(Encoder::instance().convertToPlainAscii(author));
            /// Thus, searching just for the name's text without telling API that this is a name
            queryString += QString(QStringLiteral(" %1")).arg(Encoder::instance().convertToPlainAscii(author));
        }

        QString year = query[QueryKey::Year];
        if (!year.isEmpty()) {
            static const QRegularExpression yearRegExp(QStringLiteral("\\b(18|19|20)[0-9]{2}\\b"));
            const QRegularExpressionMatch yearRegExpMatch = yearRegExp.match(year);
            if (yearRegExpMatch.hasMatch()) {
                year = yearRegExpMatch.captured(0);
                /// Query parameter 'year:' is a Premium Plan feature at Springer Nature.
                // queryString += QString(QStringLiteral(" year:%1")).arg(year);
                /// Thus, searching just for the year's text without telling API that this is a year
                queryString += QString(QStringLiteral(" %1")).arg(year);
            }
        }

        queryString = queryString.simplified();
        QUrlQuery q(queryUrl);
        q.addQueryItem(QStringLiteral("q"), queryString);
        q.addQueryItem(QStringLiteral("p"), QString::number(qMin(1, qMax(25, numResults)))); //< Enforce sane number of request (max 25 for free Basic plan)
        q.addQueryItem(QStringLiteral("api_key"), springerMetadataKey);
        queryUrl.setQuery(q);

        return queryUrl;
    }

    static QByteArray rewriteXMLformatting(const QByteArray &input) {
        QByteArray result{input};
        static const QSet<const char *> lookInsideTags{"xhtml:body", "dc:title"};

        int p1 = -1;
        while ((p1 = result.indexOf('<', p1 + 1)) >= 0) {
            bool surroundingTagFound = false;
            for (const char *surroundingTag : lookInsideTags) {
                if (qstrncmp(surroundingTag, result.constData() + p1 + 1, qstrlen(surroundingTag)) == 0)
                {
                    surroundingTagFound = true;
                    p1 = result.indexOf("<", p1 + 7);
                    while (p1 >= 0) {
                        if (result.length() > p1 + 1 && result[p1 + 1] == '/' && qstrncmp(surroundingTag, result.constData() + p1 + 2, qstrlen(surroundingTag)) == 0)
                            break;

                        bool encounteredKnowTag = false;

                        // There are some HTML tags like <h1>..</h1> or <p>..</p> that shall get removed, but not any text inside
                        // Exception are headings with 'standard' text like <h1>Abstract</h1> which is to be removed completely
                        static const QSet<const char *> ignoredTags{"h1", "h2", "h3", "p"};
                        const int tagOffset = result[p1 + 1] == '/' ? p1 + 2 : p1 + 1;
                        for (const char *tag : ignoredTags) {
                            if (qstrncmp(tag, result.constData() + tagOffset, qstrlen(tag)) == 0) {
                                int p2 = result.indexOf(">", tagOffset);
                                if (qstrncmp(result.constData() + p2 + 1, "Abstract<", 9) == 0 || qstrncmp(result.constData() + p2 + 1, "A bstract<", 10) == 0) {
                                    p2 = result.indexOf(">", p2 + 9);
                                    result = result.left(p1) + result.mid(p2 + 1);
                                } else {
                                    const char *replacement = (result[p1 + 1] == '/' && qstrncmp(tag, "p", 1) == 0) ? "\n" : "";
                                    result = result.left(p1) + replacement + result.mid(p2 + 1);
                                }
                                encounteredKnowTag = true;
                                --p1;
                                break;
                            }
                        }

                        // Remove some tags including their content, for example <CitationRef ..>..</CitationRef>
                        static const QSet<const char *> removeTagsWithContent{"CitationRef"};
                        if (!encounteredKnowTag)
                            for (const char *tag : removeTagsWithContent) {
                                if (qstrncmp(tag, result.constData() + p1 + 1, qstrlen(tag)) == 0) {
                                    int p2 = p1;
                                    while ((p2 = result.indexOf('<', p2 + 1)) >= 0) {
                                        if (result.length() > p2 + 1 && result[p2 + 1] == '/' && qstrncmp(tag, result.constData() + p2 + 2, qstrlen(tag)) == 0)
                                            break;
                                    }
                                    if (p2 > p1)
                                        p2 = result.indexOf('>', p2 + 1);
                                    if (p2 > p1) {
                                        result = result.left(p1) + result.mid(p2 + 1);
                                        encounteredKnowTag = true;
                                        --p1;
                                    } else
                                        qCWarning(LOG_KBIBTEX_NETWORKING) << "Could not find closing tag for" << tag;
                                    break;
                                }
                            }

                        // Special treatment of <InlineEquation>, which possibly contains some TeX code that should be preserved
                        if (!encounteredKnowTag && qstrncmp("InlineEquation", result.constData() + p1 + 1, 14) == 0) {
                            const int pmax = result.indexOf("</InlineEquation", p1 + 5);
                            if (pmax > p1) {
                                const int pend = result.indexOf('>', pmax + 12);
                                int p2 = result.indexOf("Format=\"TEX\"", p1 + 4);
                                if (p1 < p2 && p2 < pmax && pmax < pend) {
                                    p2 = result.indexOf('>', p2 + 9);
                                    const int p3 = result.indexOf('<', p2 + 1);
                                    if (p1 < p2 && p2 < p3 && p3 < pmax) {
                                        QByteArray equation = result.mid(p2 + 1, p3 - p2 - 1);
                                        if (equation.length() > 4 && equation.startsWith("$$") && equation.endsWith("$$"))
                                            equation = equation.mid(1, equation.length() - 2);
                                        if (equation.length() > 4 && equation.startsWith("$ ") && equation.endsWith(" $"))
                                            equation = "$" + equation.mid(2, equation.length() - 4).trimmed() + "$";
                                        result = result.left(p1) + equation + result.mid(pend + 1);
                                        encounteredKnowTag = true;
                                        --p1;
                                    }
                                }
                            }
                        }

                        // Special treatment of <Emphasis>, which possibly contains some TeX code that should be preserved
                        if (!encounteredKnowTag && qstrncmp("Emphasis", result.constData() + p1 + 1, 8) == 0) {
                            const int pmax = result.indexOf("</Emphasis", p1 + 5);
                            if (pmax > p1) {
                                const int pend = result.indexOf('>', pmax + 8);
                                const int p2 = result.indexOf('>', p1 + 8);
                                if (p1 < p2 && p2 < pmax && pmax < pend) {
                                    const int p3 = result.indexOf("Type=\"Italic\"", p1 + 6);
                                    const char *emphCommand = (p3 > p1 && p3 < pmax) ? "\\textit{" : "\\emph{";
                                    result = result.left(p1) + emphCommand + result.mid(p2 + 1, pmax - p2 - 1) + "}" + result.mid(pend + 1);
                                    encounteredKnowTag = true;
                                    --p1;
                                }
                            }
                        }

                        // Special treatment of <Superscript>, which possibly contains some TeX code that should be preserved
                        if (!encounteredKnowTag && qstrncmp("Superscript", result.constData() + p1 + 1, 11) == 0) {
                            const int pmax = result.indexOf("</Superscript", p1 + 5);
                            if (pmax > p1) {
                                const int pend = result.indexOf('>', pmax + 8);
                                const int p2 = result.indexOf('>', p1 + 8);
                                const bool isCitationRef = qstrncmp(result.constData() + p1 + 13, "<CitationRef", 12) == 0;
                                if (p1 < p2 && p2 < pmax && pmax < pend) {
                                    result = result.left(p1) + (isCitationRef ? QByteArray() : ("\\textsuperscript{" + result.mid(p2 + 1, pmax - p2 - 1) + "}")) + result.mid(pend + 1);
                                    encounteredKnowTag = true;
                                    --p1;
                                }
                            }
                        }

                        // Special treatment of <Subscript>, which possibly contains some TeX code that should be preserved
                        if (!encounteredKnowTag && qstrncmp("Subscript", result.constData() + p1 + 1, 8) == 0) {
                            const int pmax = result.indexOf("</Subscript", p1 + 5);
                            if (pmax > p1) {
                                const int pend = result.indexOf('>', pmax + 8);
                                const int p2 = result.indexOf('>', p1 + 8);
                                if (p1 < p2 && p2 < pmax && pmax < pend) {
                                    result = result.left(p1) + "\\textsubscript{" + result.mid(p2 + 1, pmax - p2 - 1) + "}" + result.mid(pend + 1);
                                    encounteredKnowTag = true;
                                    --p1;
                                }
                            }
                        }

                        if (!encounteredKnowTag) {
                            // Encountered a tag that is neither known to be ignored or removed including content
                            qCWarning(LOG_KBIBTEX_NETWORKING) << "Found unknown tag in SpringerLink data:" << result.mid(p1, 64);
                        }
                        p1 = result.indexOf("<", p1 + 1);
                    }
                    break;
                }
            }
            if (!surroundingTagFound)
                ++p1;
        }

        return OnlineSearchAbstract::htmlEntityToUnicode(result);
    }

    QVector<QSharedPointer<Entry>> parsePAM(const QByteArray &xmlData, bool *ok) {
        QVector<QSharedPointer<Entry>> result;

        // Source code generated by Python script 'onlinesearch-parser-generator.py'
        // using information from configuration file 'onlinesearchspringerlink-parser.in.cpp'
        #include "onlinesearch/onlinesearchspringerlink-parser.generated.cpp"

        return result;
    }
};


const QString OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate::springerLinkQueryBaseUrl{QStringLiteral("https://api.springernature.com/metadata/pam")};
const QString OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate::springerMetadataKey(InternalNetworkAccessManager::reverseObfuscate("\xce\xb8\x4d\x2c\x8d\xba\xa9\xc4\x61\x9\x58\x6c\xbb\xde\x86\xb5\xb1\xc6\x15\x71\x76\x45\xd\x79\x12\x65\x95\xe1\x5d\x2f\x1d\x24\x10\x72\x2a\x5e\x69\x4\xdc\xba\xab\xc3\x28\x58\x8a\xfa\x5e\x69"));

OnlineSearchSpringerLink::OnlineSearchSpringerLink(QObject *parent)
        : OnlineSearchAbstract(parent), d(new OnlineSearchSpringerLink::OnlineSearchSpringerLinkPrivate(this))
{
    /// nothing
}

OnlineSearchSpringerLink::~OnlineSearchSpringerLink()
{
    delete d;
}

#ifdef HAVE_QTWIDGETS
void OnlineSearchSpringerLink::startSearchFromForm()
{
    m_hasBeenCanceled = false;
    Q_EMIT progress(curStep = 0, numSteps = 1);

    QUrl springerLinkSearchUrl = d->buildQueryUrl();

    QNetworkRequest request(springerLinkSearchUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchSpringerLink::doneFetchingPAM);

    if (d->form != nullptr) d->form->saveState();

    refreshBusyProperty();
}
#endif // HAVE_QTWIDGETS

void OnlineSearchSpringerLink::startSearch(const QMap<QueryKey, QString> &query, int numResults)
{
    m_hasBeenCanceled = false;

    QUrl springerLinkSearchUrl = d->buildQueryUrl(query, numResults);

    Q_EMIT progress(curStep = 0, numSteps = 1);
    QNetworkRequest request(springerLinkSearchUrl);
    QNetworkReply *reply = InternalNetworkAccessManager::instance().get(request);
    InternalNetworkAccessManager::instance().setNetworkReplyTimeout(reply);
    connect(reply, &QNetworkReply::finished, this, &OnlineSearchSpringerLink::doneFetchingPAM);

    refreshBusyProperty();
}

QString OnlineSearchSpringerLink::label() const
{
#ifdef HAVE_KF
    return i18n("Springer Nature Link");
#else // HAVE_KF
    //= onlinesearch-springerlink-label
    return QObject::tr("Springer Nature Link");
#endif // HAVE_KF
}

#ifdef HAVE_QTWIDGETS
OnlineSearchAbstract::Form *OnlineSearchSpringerLink::customWidget(QWidget *parent)
{
    if (d->form == nullptr)
        d->form = new Form(parent);
    return d->form;
}
#endif // HAVE_QTWIDGETS

QUrl OnlineSearchSpringerLink::homepage() const
{
    return QUrl(QStringLiteral("https://link.springer.com/"));
}

#ifdef BUILD_TESTING
QVector<QSharedPointer<Entry> > OnlineSearchSpringerLink::parsePAM(const QByteArray &xmlData, bool *ok)
{
    return d->parsePAM(xmlData, ok);
}

QByteArray OnlineSearchSpringerLink::rewriteXMLformatting(const QByteArray &input)
{
    return OnlineSearchSpringerLinkPrivate::rewriteXMLformatting(input);
}
#endif // BUILD_TESTING

void OnlineSearchSpringerLink::doneFetchingPAM()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    if (handleErrors(reply)) {
        const QByteArray xmlCode{OnlineSearchSpringerLinkPrivate::rewriteXMLformatting(reply->readAll())};

        bool ok = false;
        const QVector<QSharedPointer<Entry>> entries = d->parsePAM(xmlCode, &ok);

        if (ok) {
            for (const auto &entry : entries)
                publishEntry(entry);
            stopSearch(resultNoError);
        } else {
            qCWarning(LOG_KBIBTEX_NETWORKING) << "Failed to parse XML data from" << InternalNetworkAccessManager::removeApiKey(reply->url()).toDisplayString();
            stopSearch(resultUnspecifiedError);
        }
    }

    refreshBusyProperty();
}

#include "onlinesearchspringerlink.moc"
