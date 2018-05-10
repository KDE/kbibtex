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

void KBibTeXNetworkingTest::initTestCase()
{
    // TODO
}

QTEST_MAIN(KBibTeXNetworkingTest)

#include "kbibtexnetworkingtest.moc"
