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

#include <QtTest/QtTest>

#include "onlinesearchabstract.h"

typedef QMap<QString, QString> FormData;

class OnlineSearchDummy : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchDummy(QObject *parent);
    void startSearch(const QMap<QString, QString> &query, int numResults) override;
    QString label() const override;
    QUrl homepage() const override;

    QMap<QString, QString> formParameters_public(const QString &htmlText, int startPos);
    void sanitizeEntry_public(QSharedPointer<Entry> entry);

protected:
    QString favIconUrl() const override;
};

class KBibTeXNetworkingTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void onlineSearchAbstractFormParameters_data();
    void onlineSearchAbstractFormParameters();
    void onlineSearchAbstractSanitizeEntry_data();
    void onlineSearchAbstractSanitizeEntry();

private:
};

OnlineSearchDummy::OnlineSearchDummy(QObject *parent)
    : OnlineSearchAbstract(parent)
{
    /// nothing
}

void OnlineSearchDummy::startSearch(const QMap<QString, QString> &query, int numResults)
{
    Q_UNUSED(query)
    Q_UNUSED(numResults)
}

QString OnlineSearchDummy::label() const
{
    return QStringLiteral("Dummy Search");
}

QUrl OnlineSearchDummy::homepage() const
{
    return QUrl::fromUserInput(QStringLiteral("https://www.kde.org"));
}

QString OnlineSearchDummy::favIconUrl() const
{
    return QStringLiteral("https://www.kde.org/favicon.ico");
}

QMap<QString, QString> OnlineSearchDummy::formParameters_public(const QString &htmlText, int startPos)
{
    return formParameters(htmlText, startPos);
}

void OnlineSearchDummy::sanitizeEntry_public(QSharedPointer<Entry> entry)
{
    sanitizeEntry(entry);
}

void KBibTeXNetworkingTest::onlineSearchAbstractFormParameters_data()
{
    QTest::addColumn<QString>("htmlCode");
    QTest::addColumn<int>("startPos");
    QTest::addColumn<FormData>("expectedResult");

    QTest::newRow("Empty Form (1)") << QString() << 0 << QMap<QString, QString>();
    QTest::newRow("Empty Form (2)") << QStringLiteral("<form></form>") << 0 << QMap<QString, QString>();
    QTest::newRow("Form with text") << QStringLiteral("<form><input type=\"text\" name=\"abc\" value=\"ABC\" /></form>") << 0 << QMap<QString, QString> {{QStringLiteral("abc"), QStringLiteral("ABC")}};
    QTest::newRow("Form with text but without quotation marks") << QStringLiteral("<form><input type=text name=abc value=ABC /></form>") << 0 << QMap<QString, QString> {{QStringLiteral("abc"), QStringLiteral("ABC")}};
    QTest::newRow("Form with text and single quotation marks") << QStringLiteral("<form><input type='text' name='abc' value='ABC' /></form>") << 0 << QMap<QString, QString> {{QStringLiteral("abc"), QStringLiteral("ABC")}};
    QTest::newRow("Form with radio button (none selected)") << QStringLiteral("<form><input type=\"radio\" name=\"direction\" value=\"right\" /><input type=\"radio\" name=\"direction\" value=\"left\"/></form>") << 0 << QMap<QString, QString>();
    QTest::newRow("Form with radio button (old-style)") << QStringLiteral("<form><input type=\"radio\" name=\"direction\" value=\"right\" /><input type=\"radio\" name=\"direction\" value=\"left\" checked/></form>") << 0 << QMap<QString, QString> {{QStringLiteral("direction"), QStringLiteral("left")}};
    QTest::newRow("Form with radio button (modern)") << QStringLiteral("<form><input type=\"radio\" name=\"direction\" value=\"right\" checked=\"checked\"/><input type=\"radio\" name=\"direction\" value=\"left\"/></form>") << 0 << QMap<QString, QString> {{QStringLiteral("direction"), QStringLiteral("right")}};
    QTest::newRow("Form with select/option (none selected)") << QStringLiteral("<form><select name=\"direction\"><option value=\"left\">Left</option><option value=\"right\">Right</option></select></form>") << 0 << QMap<QString, QString>();
    QTest::newRow("Form with select/option (old-style)") << QStringLiteral("<form><select name=\"direction\"><option value=\"left\" selected >Left</option><option value=\"right\">Right</option></select></form>") << 0 << QMap<QString, QString> {{QStringLiteral("direction"), QStringLiteral("left")}};
    QTest::newRow("Form with select/option (modern)") << QStringLiteral("<form><select name=\"direction\"><option value=\"left\" >Left</option><option selected=\"selected\" value=\"right\">Right</option></select></form>") << 0 << QMap<QString, QString> {{QStringLiteral("direction"), QStringLiteral("right")}};
}

void KBibTeXNetworkingTest::onlineSearchAbstractFormParameters()
{
    QFETCH(QString, htmlCode);
    QFETCH(int, startPos);
    QFETCH(FormData, expectedResult);

    OnlineSearchDummy onlineSearch(this);
    const FormData computedResult = onlineSearch.formParameters_public(htmlCode, startPos);

    QCOMPARE(expectedResult.size(), computedResult.size());
    for (const QString &key : expectedResult.keys()) {
        QCOMPARE(computedResult.contains(key), true);
        const QList<QString> expectedValues = expectedResult.values(key);
        const QList<QString> computedValues = computedResult.values(key);
        QCOMPARE(expectedValues.size(), computedValues.size());
        for (int p = expectedValues.size() - 1; p >= 0; --p) {
            const QString &expectedValue = expectedValues[p];
            const QString &computedValue = computedValues[p];
            QCOMPARE(expectedValue, computedValue);
        }
    }
}

void KBibTeXNetworkingTest::onlineSearchAbstractSanitizeEntry_data()
{
    QTest::addColumn<Entry *>("badInputEntry");
    QTest::addColumn<Entry *>("goodOutputEntry");

    QTest::newRow("Entry with type and id but without values") << new Entry(Entry::etArticle, QStringLiteral("abc123")) << new Entry(Entry::etArticle, QStringLiteral("abc123"));

    const Value doiValue = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("10.1000/182")));
    const Value authorValue = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("Jane Doe")));

    Entry *entryA1 = new Entry(Entry::etBook, QStringLiteral("abcdef"));
    Entry *entryA2 = new Entry(Entry::etBook, QStringLiteral("abcdef"));
    Value valueA1 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("http://dx.example.org/10.1000/182"))) << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("https://www.kde.org"))) << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("https://dx.doi.org/10.1000/183")));
    entryA1->insert(Entry::ftUrl, valueA1);
    Value valueA2 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("10.1000/182"))) << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("10.1000/183")));
    entryA2->insert(Entry::ftDOI, valueA2);
    Value valueA3 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("https://www.kde.org")));
    entryA2->insert(Entry::ftUrl, valueA3);
    QTest::newRow("Entry with DOI number in URL") << entryA1 << entryA2;

    Entry *entryB1 = new Entry(Entry::etPhDThesis, QStringLiteral("abCDef2"));
    Entry *entryB2 = new Entry(Entry::etPhDThesis, QStringLiteral("abCDef2"));
    Value valueB1 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("http://dx.example.org/10.1000/182")))  << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("https://www.kde.org"))) << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("https://dx.doi.org/10.1000/183")));
    entryB1->insert(Entry::ftUrl, valueB1);
    Value valueB2 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("10.1000/182"))) << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("10.1000/183")));
    entryB1->insert(Entry::ftDOI, valueB2);
    entryB2->insert(Entry::ftDOI, valueB2);
    Value valueB3 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("https://www.kde.org")));
    entryB2->insert(Entry::ftUrl, valueB3);
    QTest::newRow("Entry both with DOI and DOI number in URL") << entryB1 << entryB2;

    Entry *entryC1 = new Entry(Entry::etInProceedings, QStringLiteral("abc567"));
    Entry *entryC2 = new Entry(Entry::etInProceedings, QStringLiteral("abc567"));
    Value valueC1 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("42")));
    static const QString ftIssue = QStringLiteral("issue");
    entryC1->insert(ftIssue, valueC1);
    entryC1->insert(Entry::ftDOI, doiValue);
    Value valueC2 = valueC1;
    entryC2->insert(Entry::ftDOI, doiValue);
    entryC2->insert(Entry::ftNumber, valueC2);
    QTest::newRow("Entry with 'issue' becomes 'number'") << entryC1 << entryC2;

    Entry *entryD1 = new Entry(Entry::etTechReport, QStringLiteral("TR10.1000/182"));
    Entry *entryD2 = new Entry(Entry::etTechReport, QStringLiteral("TR10.1000/182"));
    entryD1->insert(Entry::ftAuthor, authorValue);
    entryD2->insert(Entry::ftDOI, doiValue);
    entryD2->insert(Entry::ftAuthor, authorValue);
    QTest::newRow("Entry's id contains DOI, set DOI field accordingly") << entryD1 << entryD2;

    Entry *entryE1 = new Entry(Entry::etMastersThesis, QStringLiteral("xyz987"));
    Entry *entryE2 = new Entry(Entry::etMastersThesis, QStringLiteral("xyz987"));
    Value valueE1 = Value() << QSharedPointer<ValueItem>(new MacroKey(QStringLiteral("TOBEREMOVED")));
    entryE1->insert(Entry::ftCrossRef, valueE1);
    entryE1->insert(Entry::ftAuthor, authorValue);
    entryE2->insert(Entry::ftAuthor, authorValue);
    QTest::newRow("Removing 'crossref' field from Entry") << entryE1 << entryE2;

    Entry *entryF1 = new Entry(Entry::etInProceedings, QStringLiteral("abc567"));
    Entry *entryF2 = new Entry(Entry::etInProceedings, QStringLiteral("abc567"));
    Value valueF1 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("Bla blubber")));
    static const QString ftDescription = QStringLiteral("description");
    entryF1->insert(ftDescription, valueF1);
    entryF1->insert(Entry::ftDOI, doiValue);
    entryF2->insert(Entry::ftDOI, doiValue);
    entryF2->insert(Entry::ftAbstract, valueF1);
    QTest::newRow("Entry with 'description' becomes 'abstract'") << entryF1 << entryF2;

    Entry *entryG1 = new Entry(Entry::etPhDThesis, QStringLiteral("qwertz"));
    Entry *entryG2 = new Entry(Entry::etPhDThesis, QStringLiteral("qwertz"));
    Value valueG1 = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("September"))) << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("/"))) << QSharedPointer<ValueItem>(new MacroKey(QStringLiteral("nov")));
    entryG1->insert(Entry::ftMonth, valueG1);
    entryG1->insert(Entry::ftDOI, doiValue);
    Value valueG2 = Value() << QSharedPointer<ValueItem>(new MacroKey(QStringLiteral("sep"))) << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("/"))) << QSharedPointer<ValueItem>(new MacroKey(QStringLiteral("nov")));
    entryG2->insert(Entry::ftDOI, doiValue);
    entryG2->insert(Entry::ftMonth, valueG2);
    QTest::newRow("Entry with month 'September' becomes macro key 'sep'") << entryG1 << entryG2;
}

void KBibTeXNetworkingTest::onlineSearchAbstractSanitizeEntry()
{
    QFETCH(Entry *, badInputEntry);
    QFETCH(Entry *, goodOutputEntry);
    QSharedPointer<Entry> badInputEntrySharedPointer(badInputEntry);

    OnlineSearchDummy onlineSearch(this);
    onlineSearch.sanitizeEntry_public(badInputEntrySharedPointer);
    QCOMPARE(*badInputEntrySharedPointer.data(), *goodOutputEntry);
    delete goodOutputEntry;
}

void KBibTeXNetworkingTest::initTestCase()
{
    // TODO
}

QTEST_MAIN(KBibTeXNetworkingTest)

#include "kbibtexnetworkingtest.moc"
