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

#include <QtTest>

#include <onlinesearch/OnlineSearchAbstract>
#include <onlinesearch/OnlineSearchArXiv>
#include <onlinesearch/OnlineSearchIEEEXplore>
#include <AssociatedFiles>

typedef QMultiMap<QString, QString> FormData;

Q_DECLARE_METATYPE(AssociatedFiles::PathType)
Q_DECLARE_METATYPE(FormData)
Q_DECLARE_METATYPE(QVector<QSharedPointer<Entry>>)

class OnlineSearchDummy : public OnlineSearchAbstract
{
    Q_OBJECT

public:
    explicit OnlineSearchDummy(QObject *parent);
    void startSearch(const QMap<QueryKey, QString> &query, int numResults) override;
    QString label() const override;
    QUrl homepage() const override;

    QMultiMap<QString, QString> formParameters_public(const QString &htmlText, int startPos);
    void sanitizeEntry_public(QSharedPointer<Entry> entry);
};

class KBibTeXNetworkingTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void onlineSearchAbstractFormParameters_data();
    void onlineSearchAbstractFormParameters();
    void onlineSearchAbstractSanitizeEntry_data();
    void onlineSearchAbstractSanitizeEntry();
#ifdef BUILD_TESTING
    void onlineSearchArXivAtomRSSparsing_data();
    void onlineSearchArXivAtomRSSparsing();
    void onlineSearchIeeeXMLparsing_data();
    void onlineSearchIeeeXMLparsing();
#endif // BUILD_TESTING

    void associatedFilescomputeAssociateURL_data();
    void associatedFilescomputeAssociateURL();
private:
};

OnlineSearchDummy::OnlineSearchDummy(QObject *parent)
        : OnlineSearchAbstract(parent)
{
    /// nothing
}

void OnlineSearchDummy::startSearch(const QMap<QueryKey, QString> &query, int numResults)
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

QMultiMap<QString, QString> OnlineSearchDummy::formParameters_public(const QString &htmlText, int startPos)
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

    QTest::newRow("Empty Form (1)") << QString() << 0 << FormData();
    QTest::newRow("Empty Form (2)") << QStringLiteral("<form></form>") << 0 << FormData();
    QTest::newRow("Form with text") << QStringLiteral("<form><input type=\"text\" name=\"abc\" value=\"ABC\" /></form>") << 0 << FormData {{QStringLiteral("abc"), QStringLiteral("ABC")}};
    QTest::newRow("Form with text but without quotation marks") << QStringLiteral("<form><input type=text name=abc value=ABC /></form>") << 0 << FormData {{QStringLiteral("abc"), QStringLiteral("ABC")}};
    QTest::newRow("Form with text and single quotation marks") << QStringLiteral("<form><input type='text' name='abc' value='ABC' /></form>") << 0 << FormData {{QStringLiteral("abc"), QStringLiteral("ABC")}};
    QTest::newRow("Form with radio button (none selected)") << QStringLiteral("<form><input type=\"radio\" name=\"direction\" value=\"right\" /><input type=\"radio\" name=\"direction\" value=\"left\"/></form>") << 0 << FormData();
    QTest::newRow("Form with radio button (old-style)") << QStringLiteral("<form><input type=\"radio\" name=\"direction\" value=\"right\" /><input type=\"radio\" name=\"direction\" value=\"left\" checked/></form>") << 0 << FormData {{QStringLiteral("direction"), QStringLiteral("left")}};
    QTest::newRow("Form with radio button (modern)") << QStringLiteral("<form><input type=\"radio\" name=\"direction\" value=\"right\" checked=\"checked\"/><input type=\"radio\" name=\"direction\" value=\"left\"/></form>") << 0 << FormData {{QStringLiteral("direction"), QStringLiteral("right")}};
    QTest::newRow("Form with select/option (none selected)") << QStringLiteral("<form><select name=\"direction\"><option value=\"left\">Left</option><option value=\"right\">Right</option></select></form>") << 0 << FormData();
    QTest::newRow("Form with select/option (old-style)") << QStringLiteral("<form><select name=\"direction\"><option value=\"left\" selected >Left</option><option value=\"right\">Right</option></select></form>") << 0 << FormData {{QStringLiteral("direction"), QStringLiteral("left")}};
    QTest::newRow("Form with select/option (modern)") << QStringLiteral("<form><select name=\"direction\"><option value=\"left\" >Left</option><option selected=\"selected\" value=\"right\">Right</option></select></form>") << 0 << FormData {{QStringLiteral("direction"), QStringLiteral("right")}};
}

void KBibTeXNetworkingTest::onlineSearchAbstractFormParameters()
{
    QFETCH(QString, htmlCode);
    QFETCH(int, startPos);
    QFETCH(FormData, expectedResult);

    OnlineSearchDummy onlineSearch(this);
    const FormData computedResult = onlineSearch.formParameters_public(htmlCode, startPos);

    QCOMPARE(expectedResult.size(), computedResult.size());
    const QList<QString> keys = expectedResult.keys();
    for (const QString &key : keys) {
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

    const Value doiValue = Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.1000/182")));
    const Value authorValue = Value() << QSharedPointer<ValueItem>(new PlainText(QStringLiteral("Jane Doe")));

    Entry *entryA1 = new Entry(Entry::etBook, QStringLiteral("abcdef"));
    Entry *entryA2 = new Entry(Entry::etBook, QStringLiteral("abcdef"));
    Value valueA1 = Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("http://dx.example.org/10.1000/182"))) << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://www.kde.org"))) << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://dx.doi.org/10.1000/183")));
    entryA1->insert(Entry::ftUrl, valueA1);
    Value valueA2 = Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.1000/182"))) << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.1000/183")));
    entryA2->insert(Entry::ftDOI, valueA2);
    Value valueA3 = Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://www.kde.org")));
    entryA2->insert(Entry::ftUrl, valueA3);
    QTest::newRow("Entry with DOI number in URL") << entryA1 << entryA2;

    Entry *entryB1 = new Entry(Entry::etPhDThesis, QStringLiteral("abCDef2"));
    Entry *entryB2 = new Entry(Entry::etPhDThesis, QStringLiteral("abCDef2"));
    Value valueB1 = Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("http://dx.example.org/10.1000/182")))  << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://www.kde.org"))) << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://dx.doi.org/10.1000/183")));
    entryB1->insert(Entry::ftUrl, valueB1);
    Value valueB2 = Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.1000/182"))) << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.1000/183")));
    entryB1->insert(Entry::ftDOI, valueB2);
    entryB2->insert(Entry::ftDOI, valueB2);
    Value valueB3 = Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://www.kde.org")));
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

#ifdef BUILD_TESTING
void KBibTeXNetworkingTest::onlineSearchArXivAtomRSSparsing_data()
{
    QTest::addColumn<QByteArray>("xmlData");
    QTest::addColumn<bool>("expectedOk");
    QTest::addColumn<QVector<QSharedPointer<Entry>>>("expectedEntries");

    QTest::newRow("Empty input data") << QByteArray() << false << QVector<QSharedPointer<Entry>>();
    QTest::newRow("Glibberish") << QByteArrayLiteral("dfbhflbkndfsgn") << false << QVector<QSharedPointer<Entry>>();

    auto arxiv150400141v1 = QSharedPointer<Entry>(new Entry(Entry::etMisc, QStringLiteral("arXiv:1504.00141v1")));
    arxiv150400141v1->insert(Entry::ftAbstract, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("We give necessary and fppje dccrz so that we eoeml d-hypercyclicity for fnsxq who map a holomorphic function to a afczs sum of the Taylor dpcef. This jksqc is connected yaqxf doubly fzexf Taylors series and this is an lefws to gaxws the yppmz to rqqfi fzexf Taylor series."))));
    const Value valueArchivePrefix = Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("arXiv")));
    arxiv150400141v1->insert(QStringLiteral("archiveprefix"), valueArchivePrefix);
    arxiv150400141v1->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Vagia"), QStringLiteral("Vlachou"))));
    arxiv150400141v1->insert(Entry::ftDOI, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.48550/arXiv.1504.00141"))));
    arxiv150400141v1->insert(QStringLiteral("eprint"), Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("1504.00141"))));
    const Value valueMonthApril = Value() << QSharedPointer<MacroKey>(new MacroKey(QStringLiteral("apr")));
    arxiv150400141v1->insert(Entry::ftMonth, valueMonthApril);
    arxiv150400141v1->insert(QStringLiteral("primaryclass"), Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("math.CV"))));
    arxiv150400141v1->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Disjoint Hypercyclicity for ibnxz of Taylor-type Operators"))));
    arxiv150400141v1->insert(Entry::ftUrl, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("http://arxiv.org/abs/1504.00141v1"))));
    arxiv150400141v1->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2015"))));
    QTest::newRow("1504.00141v1") << QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<feed xmlns=\"http://www.w3.org/2005/Atom\">\n<link href=\"http://arxiv.org/api/query?search_query%3Dall%3A%22taylor%22%26id_list%3D%26start%3D0%26max_results%3D10\" rel=\"self\" type=\"application/atom+xml\"/><title type=\"html\">ArXiv Query: search_query=all:\"taylor\"&amp;id_list=&amp;start=0&amp;max_results=10</title><id>http://arxiv.org/api/6h9sToXCpLXkWfu8KLjT9VitsINsBm1Y</id><updated>2022-11-04T00:00:00-04:00</updated><opensearch:totalResults xmlns:opensearch=\"http://a9.com/-/spec/opensearch/1.1/\">11993</opensearch:totalResults><opensearch:startIndex xmlns:opensearch=\"http://a9.com/-/spec/opensearch/1.1/\">0</opensearch:startIndex><opensearch:itemsPerPage xmlns:opensearch=\"http://a9.com/-/spec/opensearch/1.1/\">10</opensearch:itemsPerPage><entry><id>http://arxiv.org/abs/1504.00141v1</id><updated>2015-04-01T08:25:07Z</updated><published>2015-04-01T08:25:07Z</published><title>Disjoint Hypercyclicity for ibnxz of Taylor-type Operators"
                                  "</title><summary>  We give necessary and fppje dccrz so that we eoeml d-hypercyclicity for fnsxq who map a holomorphic function to a afczs sum of the Taylor dpcef. This jksqc is connected yaqxf doubly fzexf Taylors series and this is an lefws to gaxws the yppmz to rqqfi fzexf Taylor series. </summary><author><name>Vagia Vlachou</name></author><link href=\"http://arxiv.org/abs/1504.00141v1\" rel=\"alternate\" type=\"text/html\"/><link title=\"pdf\" href=\"http://arxiv.org/pdf/1504.00141v1\" rel=\"related\" type=\"application/pdf\"/><arxiv:primary_category xmlns:arxiv=\"http://arxiv.org/schemas/atom\" term=\"math.CV\" scheme=\"http://arxiv.org/schemas/atom\"/><category term=\"math.CV\" scheme=\"http://arxiv.org/schemas/atom\"/><category term=\"47A16, 47B38, 41A30\" scheme=\"http://arxiv.org/schemas/atom\"/>\n</entry>\n</feed>") << true << QVector<QSharedPointer<Entry>> {arxiv150400141v1};

    auto arxiv09122475v2 = QSharedPointer<Entry>(new Entry(Entry::etArticle, QStringLiteral("arXiv:0912.2475v2")));
    arxiv09122475v2->insert(Entry::ftAbstract, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("We vzusx holographic superconductors in Einstein-Gauss-Bonnet gravity. We consider two wwgze backgrounds: a $d$-ceuda Gauss-Bonnet-AdS black hole and a Gauss-Bonnet-AdS soliton. We discuss in bhgtq the effects that the dmaul of the vrunw field, the Gauss-Bonnet coupling and the dimensionality of the AdS space eoeml on the jomwq rnsbo and conductivity. We also vzusx the ratio $\\omega_g/T_c $ for various masses of the vrunw field and Gauss-Bonnet couplings."))));
    arxiv09122475v2->insert(QStringLiteral("archiveprefix"), valueArchivePrefix);
    arxiv09122475v2->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Qiyuan"), QStringLiteral("Pan"))) << QSharedPointer<Person>(new Person(QStringLiteral("Bin"), QStringLiteral("Wang"))) << QSharedPointer<Person>(new Person(QStringLiteral("Eleftherios"), QStringLiteral("Papantonopoulos"))) << QSharedPointer<Person>(new Person(QStringLiteral("Jeferson"), QStringLiteral("de Oliveira"))) << QSharedPointer<Person>(new Person(QStringLiteral("A. B."), QStringLiteral("Pavan"))));
    arxiv09122475v2->insert(Entry::ftDOI, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.1103/PhysRevD.81.106007"))));
    arxiv09122475v2->insert(QStringLiteral("eprint"), Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("0912.2475"))));
    arxiv09122475v2->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Phys.Rev.D"))));
    arxiv09122475v2->insert(Entry::ftMonth, valueMonthApril);
    arxiv09122475v2->insert(Entry::ftNote, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("21 pages, 10 figures. accepted for publication in PRD"))));
    arxiv09122475v2->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("106007"))));
    arxiv09122475v2->insert(QStringLiteral("primaryclass"), Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("hep-th"))));
    arxiv09122475v2->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Holographic Superconductors yaqxf various condensates in Einstein-Gauss-Bonnet gravity"))));
    arxiv09122475v2->insert(Entry::ftUrl, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("http://arxiv.org/abs/0912.2475v2"))));
    arxiv09122475v2->insert(Entry::ftVolume, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("81"))));
    arxiv09122475v2->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2010"))));
    QTest::newRow("1504.00141v1 & 0912.2475v2") << QByteArrayLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<feed xmlns=\"http://www.w3.org/2005/Atom\">\n<link href=\"http://arxiv.org/api/query?search_query%3Dall%3A%22taylor%22%26id_list%3D%26start%3D0%26max_results%3D10\" rel=\"self\" type=\"application/atom+xml\"/><title type=\"html\">ArXiv Query: search_query=all:\"taylor\"&amp;id_list=&amp;start=0&amp;max_results=10</title><id>http://arxiv.org/api/6h9sToXCpLXkWfu8KLjT9VitsINsBm1Y</id><updated>2022-11-04T00:00:00-04:00</updated><opensearch:totalResults xmlns:opensearch=\"http://a9.com/-/spec/opensearch/1.1/\">11993</opensearch:totalResults><opensearch:startIndex xmlns:opensearch=\"http://a9.com/-/spec/opensearch/1.1/\">0</opensearch:startIndex><opensearch:itemsPerPage xmlns:opensearch=\"http://a9.com/-/spec/opensearch/1.1/\">10</opensearch:itemsPerPage><entry><id>http://arxiv.org/abs/1504.00141v1</id><updated>2015-04-01T08:25:07Z</updated><published>2015-04-01T08:25:07Z</published><title>Disjoint Hypercyclicity for ibnxz of Taylor-type Operators"
            "</title><summary>  We give necessary and fppje dccrz so that we eoeml d-hypercyclicity for fnsxq who map a holomorphic function to a afczs sum of the Taylor dpcef. This jksqc is connected yaqxf doubly fzexf Taylors series and this is an lefws to gaxws the yppmz to rqqfi fzexf Taylor series. </summary><author><name>Vagia Vlachou</name></author><link href=\"http://arxiv.org/abs/1504.00141v1\" rel=\"alternate\" type=\"text/html\"/><link title=\"pdf\" href=\"http://arxiv.org/pdf/1504.00141v1\" rel=\"related\" type=\"application/pdf\"/><arxiv:primary_category xmlns:arxiv=\"http://arxiv.org/schemas/atom\" term=\"math.CV\" scheme=\"http://arxiv.org/schemas/atom\"/><category term=\"math.CV\" scheme=\"http://arxiv.org/schemas/atom\"/><category term=\"47A16, 47B38, 41A30\" scheme=\"http://arxiv.org/schemas/atom\"/>\n</entry>\n<entry><id>http://arxiv.org/abs/0912.2475v2</id><updated>2010-04-09T05:27:26Z</updated><published>2009-12-13T05:11:02Z</published><title>"
            "Holographic Superconductors yaqxf various condensates in Einstein-Gauss-Bonnet gravity</title><summary>  We vzusx holographic superconductors in Einstein-Gauss-Bonnet gravity. We consider two wwgze backgrounds: a $d$-ceuda Gauss-Bonnet-AdS black hole and a Gauss-Bonnet-AdS soliton. We discuss in bhgtq the effects that the dmaul of the vrunw field, the Gauss-Bonnet coupling and the dimensionality of the AdS space eoeml on the jomwq rnsbo and conductivity. We also vzusx the ratio $\\omega_g/T_c $ for various masses of the vrunw field and Gauss-Bonnet couplings.</summary><author><name>Qiyuan Pan</name></author><author><name>Bin Wang</name></author><author><name>Eleftherios Papantonopoulos</name></author><author><name>Jeferson de Oliveira</name></author><author><name>A. B. Pavan</name></author><arxiv:doi xmlns:arxiv=\"http://arxiv.org/schemas/atom\">10.1103/PhysRevD.81.106007</arxiv:doi><link title=\"doi\" href=\"http://dx.doi.org/10.1103/PhysRevD.81.106007\" rel=\"related\"/>"
            "<arxiv:comment xmlns:arxiv=\"http://arxiv.org/schemas/atom\">21 pages, 10 figures. accepted for publication in PRD</arxiv:comment><arxiv:journal_ref xmlns:arxiv=\"http://arxiv.org/schemas/atom\">Phys.Rev.D81:106007,2010</arxiv:journal_ref><link href=\"http://arxiv.org/abs/0912.2475v2\" rel=\"alternate\" type=\"text/html\"/><link title=\"pdf\" href=\"http://arxiv.org/pdf/0912.2475v2\" rel=\"related\" type=\"application/pdf\"/><arxiv:primary_category xmlns:arxiv=\"http://arxiv.org/schemas/atom\" term=\"hep-th\" scheme=\"http://arxiv.org/schemas/atom\"/><category term=\"hep-th\" scheme=\"http://arxiv.org/schemas/atom\"/><category term=\"gr-qc\" scheme=\"http://arxiv.org/schemas/atom\"/></entry>\n</feed>") << true << QVector<QSharedPointer<Entry>> {arxiv150400141v1, arxiv09122475v2};
}

void KBibTeXNetworkingTest::onlineSearchArXivAtomRSSparsing()
{
    QFETCH(QByteArray, xmlData);
    QFETCH(bool, expectedOk);
    QFETCH(QVector<QSharedPointer<Entry>>, expectedEntries);

    OnlineSearchArXiv osa(this);
    bool ok = false;
    const auto generatedEntries = osa.parseAtomXML(xmlData, &ok);
    QCOMPARE(expectedOk, ok);
    QCOMPARE(generatedEntries.length(), expectedEntries.length());
    if (ok) {
        for (auto itA = expectedEntries.constBegin(), itB = generatedEntries.constBegin(); itA != expectedEntries.constEnd() && itB != generatedEntries.constEnd(); ++itA, ++itB) {
            const QSharedPointer<Entry> &entryA = *itA;
            const QSharedPointer<Entry> &entryB = *itB;
            QCOMPARE(*entryA, *entryB);
        }
    }
}

void KBibTeXNetworkingTest::onlineSearchIeeeXMLparsing_data()
{
    QTest::addColumn<QByteArray>("xmlData");
    QTest::addColumn<bool>("expectedOk");
    QTest::addColumn<QVector<QSharedPointer<Entry>>>("expectedEntries");

    QTest::newRow("Empty input data") << QByteArray() << false << QVector<QSharedPointer<Entry>>();
    QTest::newRow("Glibberish") << QByteArrayLiteral("dfbhflbkndfsgn") << false << QVector<QSharedPointer<Entry>>();

    auto ieee7150835 = QSharedPointer<Entry>(new Entry(Entry::etInProceedings, QStringLiteral("ieee7150835")));
    ieee7150835->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("An analysis on data Accountability and Security in jjzyg"))));
    ieee7150835->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Shital A."), QStringLiteral("Hande"))) << QSharedPointer<Person>(new Person(QStringLiteral("Sunil B."), QStringLiteral("Mane"))));
    ieee7150835->insert(Entry::ftBookTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2015 International Conference on Industrial Instrumentation and Control (ICIC)"))));
    ieee7150835->insert(Entry::ftPublisher, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("IEEE"))));
    ieee7150835->insert(Entry::ftAbstract, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Cloud ydfue is one of the best ecrji parameter in IT zrrdf which wkmmt on awrtb tyzef. It uses Central ntppr xzcym and internet for pxict data and cxzlz. Users can opdmd their files and cxzlz hpggf internet opdmd. As it is publically available so data ihroa must be ssyxu. In this ftkab, we bkeuc ixalo izcrf ihroa and wrtsw djqmw of jjzyg ydfue such as integrity, confidentiality, otmyd, dqgnm, ymace, lugjg There are many ihroa issues related to jjzyg ydfue which includes users data xhkrp remotely by many users, user uxidm know how their data get processed on to the jjzyg. To deal with it many ihroa and dqgnm models exit. Here we ykpbs some of them like Privacy kdmce in jjzyg, RSA based storage ihroa system, Cloud ydfue data ihroa model and Cloud clyoc dqgnm knasm. We bkeuc ixalo these four models and elfse ytsoq that which model is hevvm for which type of wrtsw or service. Proposed system nmyek on nkpgq Lightweight Framework for Accountability and Security."))));
    ieee7150835->insert(Entry::ftMonth, Value() << QSharedPointer<MacroKey>(new MacroKey(QStringLiteral("may"))));
    ieee7150835->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2015"))));
    ieee7150835->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("713\u2013717"))));
    ieee7150835->insert(Entry::ftISBN, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("978-1-4799-7164-0"))));
    ieee7150835->insert(Entry::ftKeywords, Value() << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Cloud ydfue"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Data ihroa"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Data models"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Computational modeling"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Servers"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Data privacy"))));
    ieee7150835->insert(Entry::ftDOI, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("10.1109/IIC.2015.7150835"))));
    ieee7150835->insert(Entry::ftUrl, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://ieeexplore.ieee.org/document/7150835/"))));
    QTest::newRow("ieee7150835") << QByteArrayLiteral("<articles><totalfound>1</totalfound><totalsearched>5942285</totalsearched><article><doi>10.1109/IIC.2015.7150835</doi><title>An analysis on data Accountability and Security in jjzyg</title><publisher>IEEE</publisher><isbn>978-1-4799-7164-0</isbn><rank>1</rank><authors><author><affiliation>Department of Computer Engineering and IT, College of Engineering, Pune</affiliation><authorUrl>https://ieeexplore.ieee.org/author/37085452758</authorUrl><id>37085452758</id><full_name>Shital A. Hande</full_name><author_order>1</author_order><authorAffiliations><authorAffiliation>Department of Computer Engineering and IT, College of Engineering, Pune</authorAffiliation></authorAffiliations></author><author><affiliation>Department of Computer Engineering and IT, College of Engineering, Pune</affiliation><authorUrl>https://ieeexplore.ieee.org/author/37603496400</authorUrl><id>37603496400</id><full_name>Sunil B. Mane</full_name><author_order>2</author_order><authorAffiliations><authorAffiliation>Department"
                                 " of Computer Engineering and IT, College of Engineering, Pune</authorAffiliation></authorAffiliations></author></authors><accessType>locked</accessType><content_type>Conferences</content_type><abstract>Cloud ydfue is one of the best ecrji parameter in IT zrrdf which wkmmt on awrtb tyzef. It uses Central ntppr xzcym and internet for pxict data and cxzlz. Users can opdmd their files and cxzlz hpggf internet opdmd. As it is publically available so data ihroa must be ssyxu. In this ftkab, we bkeuc ixalo izcrf ihroa and wrtsw djqmw of jjzyg ydfue such as integrity, confidentiality, otmyd, dqgnm, ymace, lugjg There are many ihroa issues related to jjzyg ydfue which includes users data xhkrp remotely by many users, user uxidm know how their data get processed on to the jjzyg. To deal with it many ihroa and dqgnm models exit. Here we ykpbs some of them like Privacy kdmce"
                                 " in jjzyg, RSA based storage ihroa system, Cloud ydfue data ihroa model and Cloud clyoc dqgnm knasm. We bkeuc ixalo these four models and elfse ytsoq that which model is hevvm for which type of wrtsw or service. Proposed system nmyek on nkpgq Lightweight Framework for Accountability and Security.</abstract><article_number>7150835</article_number><pdf_url>https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&amp;arnumber=7150835</pdf_url><html_url>https://ieeexplore.ieee.org/document/7150835/</html_url><abstract_url>https://ieeexplore.ieee.org/document/7150835/</abstract_url><publication_title>2015 International Conference on Industrial Instrumentation and Control (ICIC)</publication_title><conference_location>Pune, India</conference_location><conference_dates>28-30 May 2015</conference_dates><publication_number>7133193</publication_number><is_number>7150576</is_number><publication_year>2015</publication_year><publication_date>28-30 May 2015"
                                 "</publication_date><start_page>713</start_page><end_page>717</end_page><citing_paper_count>3</citing_paper_count><citing_patent_count>0</citing_patent_count><download_count>456</download_count><insert_date>20150709</insert_date><index_terms><ieee_terms><term>Cloud ydfue</term><term>Data ihroa</term><term>Data models</term><term>Computational modeling</term><term>Servers</term><term>Data privacy</term></ieee_terms><author_terms><terms>Cloud ydfue</terms><terms>Cloud Accountability</terms><terms>data ihroa</terms></author_terms></index_terms><isbn_formats><isbn_format><format>DVD ISBN</format><value>978-1-4799-7164-0</value><isbnType>New-2005</isbnType></isbn_format><isbn_format><format>Electronic ISBN</format><value>978-1-4799-7165-7</value><isbnType>New-2005</isbnType></isbn_format></isbn_formats></article></articles>") << true << QVector<QSharedPointer<Entry>> {ieee7150835};

    auto ieee9898655 = QSharedPointer<Entry>(new Entry(Entry::etInBook, QStringLiteral("ieee9898655")));
    ieee9898655->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Secure Development"))));
    ieee9898655->insert(Entry::ftBookTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Start-Up Secure: Baking Cybersecurity into Your Company from Founding to Exit"))));
    ieee9898655->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Chris"), QStringLiteral("Castaldo"))));
    ieee9898655->insert(Entry::ftPublisher, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Wiley"))));
    ieee9898655->insert(Entry::ftAbstract, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Building a minimally ftwgz product uxidm bkeuc some basics of ihroa baked into it. Secure coding is not always a rvesx cdfxa but many modern tools of today help to enable developers to wbacv xwszt pvrte. When it ghjet to nkpgq software there is no kgxru of yfbvx to do so. Building Security in Maturity Model is anynn to be aware of rsbps the npdgp and xnfkd phase and anynn to mqefv consider in jrsuy phase. The Capability Maturity Model Integration (CMMI) is a long\u2010""standing model for software alvtu jowkf. CMMI is notable as it is yguju a requirement on US wdwph software alvtu xdmwf. When ituab are nkpgq code in their Integrated Development Environment (IDE) there are now many yidqp tools that add on to gfoxb all the popular IDEs available today that act as spell\u2010""checkers for secure code."))));
    ieee9898655->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2021"))));
    ieee9898655->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("143\u2013151"))));
    ieee9898655->insert(Entry::ftISBN, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("9781119700746"))));
    ieee9898655->insert(Entry::ftKeywords, Value() << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Capability jowkf model"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Codes"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Testing"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Training"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Computer ihroa"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Buildings"))) << QSharedPointer<Keyword>(new Keyword(QStringLiteral("Standards organizations"))));
    ieee9898655->insert(Entry::ftUrl, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("https://ieeexplore.ieee.org/document/9898655/"))));
    QTest::newRow("ieee9898655") << QByteArrayLiteral("<articles><totalfound>1</totalfound><totalsearched>5942285</totalsearched><article><title>Secure Development</title><publisher>Wiley</publisher><isbn>9781119700746</isbn><rank>1</rank><authors><author><full_name>Chris Castaldo</full_name><author_order>1</author_order><authorAffiliations><authorAffiliation/></authorAffiliations></author></authors><accessType>locked</accessType><content_type>Books</content_type><abstract>Building a minimally ftwgz product uxidm bkeuc some basics of ihroa baked into it. Secure coding is not always a rvesx cdfxa but many modern tools of today help to enable developers to wbacv xwszt pvrte. When it ghjet to nkpgq software there is no kgxru of yfbvx to do so. Building Security in Maturity Model is anynn to be aware of rsbps the npdgp and xnfkd phase and anynn to mqefv consider in jrsuy phase. The Capability Maturity Model Integration (CMMI) is a long&amp;#x2010;standing model for software alvtu jowkf."
                                 " CMMI is notable as it is yguju a requirement on US wdwph software alvtu xdmwf. When ituab are nkpgq code in their Integrated Development Environment (IDE) there are now many yidqp tools that add on to gfoxb all the popular IDEs available today that act as spell&amp;#x2010;checkers for secure code.</abstract><article_number>9898655</article_number><pdf_url>https://ieeexplore.ieee.org/xpl/ebooks/bookPdfWithBanner.jsp?fileName=9898655.pdf&amp;bkn=9872307&amp;pdfType=chapter</pdf_url><html_url>https://ieeexplore.ieee.org/document/9898655/</html_url><abstract_url>https://ieeexplore.ieee.org/document/9898655/</abstract_url><publication_title>Start-Up Secure: Baking Cybersecurity into Your Company from Founding to Exit</publication_title><publication_number>9872307</publication_number><publication_year>2021</publication_year><start_page>143</start_page><end_page>151</end_page><citing_paper_count>0</citing_paper_count><citing_patent_count>0</citing_patent_count>"
                                 "<download_count>7</download_count><insert_date>20220921</insert_date><index_terms><ieee_terms><term>Capability jowkf model</term><term>Codes</term><term>Testing</term><term>Training</term><term>Computer ihroa</term><term>Buildings</term><term>Standards organizations</term></ieee_terms></index_terms><isbn_formats><isbn_format><format>Electronic ISBN</format><value>9781119700746</value><isbnType>New-2005</isbnType></isbn_format><isbn_format><format>Online ISBN</format><value>9781394174768</value><isbnType>New-2005</isbnType></isbn_format><isbn_format><format>Print ISBN</format><value>9781119700739</value><isbnType>New-2005</isbnType></isbn_format><isbn_format><format>Electronic ISBN</format><value>9781119700753</value><isbnType>New-2005</isbnType></isbn_format></isbn_formats></article></articles>") << true << QVector<QSharedPointer<Entry>> {ieee9898655};
}

void KBibTeXNetworkingTest::onlineSearchIeeeXMLparsing()
{
    QFETCH(QByteArray, xmlData);
    QFETCH(bool, expectedOk);
    QFETCH(QVector<QSharedPointer<Entry>>, expectedEntries);

    OnlineSearchIEEEXplore searchIEEExplore(this);
    bool ok = false;
    const auto generatedEntries = searchIEEExplore.parseIeeeXML(xmlData, &ok);
    QCOMPARE(expectedOk, ok);
    QCOMPARE(generatedEntries.length(), expectedEntries.length());
    if (ok) {
        for (auto itA = expectedEntries.constBegin(), itB = generatedEntries.constBegin(); itA != expectedEntries.constEnd() && itB != generatedEntries.constEnd(); ++itA, ++itB) {
            const QSharedPointer<Entry> &entryA = *itA;
            const QSharedPointer<Entry> &entryB = *itB;
            QCOMPARE(*entryA, *entryB);
        }
    }
}
#endif // BUILD_TESTING

void KBibTeXNetworkingTest::associatedFilescomputeAssociateURL_data()
{
    QTest::addColumn<QUrl>("documentUrl");
    QTest::addColumn<File *>("bibTeXFile");
    QTest::addColumn<AssociatedFiles::PathType>("pathType");
    QTest::addColumn<QString>("expectedResult");

    File *bibTeXFile = new File();
    bibTeXFile->setProperty(File::Url, QUrl::fromUserInput(QStringLiteral("https://www.example.com/bibliography/all.bib")));
    QTest::newRow("Remote URL, relative path requested") << QUrl::fromUserInput(QStringLiteral("https://www.example.com/documents/paper.pdf")) << bibTeXFile << AssociatedFiles::PathType::Relative << QStringLiteral("../documents/paper.pdf");

    bibTeXFile = new File();
    bibTeXFile->setProperty(File::Url, QUrl::fromUserInput(QStringLiteral("https://www.example.com/bibliography/all.bib")));
    QTest::newRow("Remote URL, absolute path requested") << QUrl::fromUserInput(QStringLiteral("https://www.example.com/documents/paper.pdf")) << bibTeXFile << AssociatedFiles::PathType::Absolute << QStringLiteral("https://www.example.com/documents/paper.pdf");

    bibTeXFile = new File();
    QTest::newRow("Empty base URL, relative path requested") << QUrl::fromUserInput(QStringLiteral("https://www.example.com/documents/paper.pdf")) << bibTeXFile << AssociatedFiles::PathType::Relative << QStringLiteral("https://www.example.com/documents/paper.pdf");

    bibTeXFile = new File();
    bibTeXFile->setProperty(File::Url, QUrl::fromUserInput(QStringLiteral("https://www.example.com/bibliography/all.bib")));
    QTest::newRow("Empty document URL, relative path requested") << QUrl() << bibTeXFile << AssociatedFiles::PathType::Relative << QString();

    bibTeXFile = new File();
    bibTeXFile->setProperty(File::Url, QUrl(QStringLiteral("bibliography/all.bib")));
    QTest::newRow("Document URL and base URL are relative, relative path requested") << QUrl(QStringLiteral("documents/paper.pdf")) << bibTeXFile << AssociatedFiles::PathType::Relative << QStringLiteral("documents/paper.pdf");

    bibTeXFile = new File();
    bibTeXFile->setProperty(File::Url, QUrl::fromUserInput(QStringLiteral("https://www.example.com/bibliography/all.bib")));
    QTest::newRow("Document URL and base URL have different protocols, relative path requested") << QUrl::fromUserInput(QStringLiteral("ftp://www.example.com/documents/paper.pdf")) << bibTeXFile << AssociatedFiles::PathType::Relative << QStringLiteral("ftp://www.example.com/documents/paper.pdf");

    bibTeXFile = new File();
    bibTeXFile->setProperty(File::Url, QUrl::fromUserInput(QStringLiteral("https://www.example.com/bibliography/all.bib")));
    QTest::newRow("Document URL and base URL have different hosts, relative path requested") << QUrl::fromUserInput(QStringLiteral("https://www.example2.com/documents/paper.pdf")) << bibTeXFile << AssociatedFiles::PathType::Relative << QStringLiteral("https://www.example2.com/documents/paper.pdf");
}

void KBibTeXNetworkingTest::associatedFilescomputeAssociateURL()
{
    QFETCH(QUrl, documentUrl);
    QFETCH(File *, bibTeXFile);
    QFETCH(AssociatedFiles::PathType, pathType);
    QFETCH(QString, expectedResult);

    const QString computedResult = AssociatedFiles::computeAssociateString(documentUrl, bibTeXFile, pathType);
    QCOMPARE(expectedResult, computedResult);
    delete bibTeXFile;
}

void KBibTeXNetworkingTest::initTestCase()
{
    // TODO
}

QTEST_MAIN(KBibTeXNetworkingTest)

#include "kbibtexnetworkingtest.moc"
