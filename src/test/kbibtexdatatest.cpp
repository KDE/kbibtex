/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <FileExporterBibTeX>
#include <FileImporterBibTeX>
#include <Entry>
#include <Macro>

class KBibTeXDataTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    /**
     * This test is not designed to fail under regular circumstances.
     * Instead, this test should be run with 'valgrind' to observe any
     * irregularities in memory management or access.
     */
    void createAndRemoveValueFromEntries();

    void rearrangingCrossRefEntries_data();
    void rearrangingCrossRefEntries();

private:
};


void KBibTeXDataTest::createAndRemoveValueFromEntries()
{
    static const QString keyData[] = {Entry::ftAuthor, Entry::ftISBN};

    Entry entry;
    for (const QString &key : keyData) {
        for (int i = 0; i < 2; ++i) {
            entry.remove(key);
            Value v;
            entry.insert(key, v);
        }
    }
}

void KBibTeXDataTest::rearrangingCrossRefEntries_data()
{
    QTest::addColumn<File *>("originalBibTeXfile");
    QTest::addColumn<File *>("rearrangedBibTeXfile");

    QSharedPointer<Entry> aaabbb{new Entry(Entry::etBook, QStringLiteral("aaabbb"))};
    aaabbb->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Companion of the Duality of Polymorphism}"))));
    aaabbb->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Winston"), QStringLiteral("Smith"))));
    aaabbb->insert(Entry::ftPublisher, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Black Books"))));
    aaabbb->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2005"))));
    QSharedPointer<Entry> bbbccc{new Entry(Entry::etArticle, QStringLiteral("bbbccc"))};
    bbbccc->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Glibberish}"))));
    bbbccc->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Bernard"), QStringLiteral("Marx"))));
    bbbccc->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2022"))));
    bbbccc->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("New Swabia Journal of Incomplete Algorithms"))));
    QSharedPointer<Entry> cccddd{new Entry(Entry::etArticle, QStringLiteral("cccddd"))};
    cccddd->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Nonsense}"))));
    cccddd->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Lenina"), QStringLiteral("Crowne"))));
    cccddd->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2019"))));
    cccddd->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("International Journal for the Advancement of Reversal"))));
    QSharedPointer<Entry> dddeee{new Entry(Entry::etArticle, QStringLiteral("dddeee"))};
    dddeee->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Bulls**t}"))));
    dddeee->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Thomas"), QStringLiteral("Grahambell"))));
    dddeee->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2021"))));
    dddeee->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(QString::fromUtf8("Kot und K\xc3\xb6ter"))));
    QSharedPointer<Entry> eeefff{new Entry(Entry::etArticle, QStringLiteral("eeefff"))};
    eeefff->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Bla Bla}"))));
    eeefff->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Helmholtz"), QStringLiteral("Watson"))));
    eeefff->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("1998"))));
    eeefff->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("New Journal of Old Things"))));
    QSharedPointer<Entry> fffggg{new Entry(Entry::etInBook, QStringLiteral("fffggg"))};
    fffggg->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Subsurface String Theory}"))));
    fffggg->insert(Entry::ftCrossRef, Value() << QSharedPointer<VerbatimText>(new VerbatimText(QStringLiteral("aaabbb"))));
    fffggg->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(QString::fromUtf8("42\xe2\x80\x93""56"))));

    File *originalBibTeXfile = new File();
    originalBibTeXfile->append(aaabbb);
    originalBibTeXfile->append(eeefff);
    originalBibTeXfile->append(dddeee);
    originalBibTeXfile->append(fffggg);
    originalBibTeXfile->append(bbbccc);
    originalBibTeXfile->append(cccddd);
    File *rearrangedBibTeXfile = new File();
    rearrangedBibTeXfile->append(eeefff);
    rearrangedBibTeXfile->append(dddeee);
    rearrangedBibTeXfile->append(fffggg);
    rearrangedBibTeXfile->append(bbbccc);
    rearrangedBibTeXfile->append(cccddd);
    rearrangedBibTeXfile->append(aaabbb);
    QTest::newRow("Crossref'ed to end") << originalBibTeXfile << rearrangedBibTeXfile;

    originalBibTeXfile = new File();
    originalBibTeXfile->append(eeefff);
    originalBibTeXfile->append(dddeee);
    originalBibTeXfile->append(fffggg);
    originalBibTeXfile->append(aaabbb);
    originalBibTeXfile->append(bbbccc);
    originalBibTeXfile->append(cccddd);
    rearrangedBibTeXfile = new File();
    rearrangedBibTeXfile->append(eeefff);
    rearrangedBibTeXfile->append(dddeee);
    rearrangedBibTeXfile->append(fffggg);
    rearrangedBibTeXfile->append(aaabbb);
    rearrangedBibTeXfile->append(bbbccc);
    rearrangedBibTeXfile->append(cccddd);
    QTest::newRow("Crossref'ed after reference") << originalBibTeXfile << rearrangedBibTeXfile;
}

void KBibTeXDataTest::rearrangingCrossRefEntries()
{
    QFETCH(File *, originalBibTeXfile);
    QFETCH(File *, rearrangedBibTeXfile);

    FileExporterBibTeX exporter(this);
    FileImporterBibTeX importer(this);

    File *savedBibTeXfile = importer.fromString(exporter.toString(originalBibTeXfile));

    QCOMPARE(*rearrangedBibTeXfile, *savedBibTeXfile);
}

void KBibTeXDataTest::initTestCase()
{
    // TODO
}

QTEST_MAIN(KBibTeXDataTest)

#include "kbibtexdatatest.moc"
