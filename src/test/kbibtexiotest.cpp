/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <QStandardPaths>

#include <KBibTeX>
#include <Preferences>
#include <Value>
#include <Entry>
#include <File>
#include <FileInfo>
#include <EncoderXML>
#include <EncoderLaTeX>
#include <FileImporter>
#include <FileImporterBibTeX>
#include <FileImporterRIS>
#include <FileExporterBibTeX>
#include <FileExporterRIS>
#include <FileExporterXML>
#include <FileExporterXSLT>

#include "logging_test.h"

Q_DECLARE_METATYPE(QMimeType)
Q_DECLARE_METATYPE(QSharedPointer<Element>)

class KBibTeXIOTest : public QObject
{
    Q_OBJECT

private:
    File *mobyDickBibliography();
    File *latinUmlautBibliography();
    File *koreanBibliography();
    File *russianBibliography();
    QVector<QPair<const char *, File *> > fileImporterExporterTestCases();

private slots:
    void initTestCase();
    void encoderConvertToPlainAscii_data();
    void encoderConvertToPlainAscii();
    void encoderXMLdecode_data();
    void encoderXMLdecode();
    void encoderXMLencode_data();
    void encoderXMLencode();
    void encoderLaTeXdecode_data();
    void encoderLaTeXdecode();
    void encoderLaTeXencode_data();
    void encoderLaTeXencode();
    void encoderLaTeXencodeHash();
    void fileImporterSplitName_data();
    void fileImporterSplitName();
    void fileInfoMimeTypeForUrl_data();
    void fileInfoMimeTypeForUrl();
    void fileInfoUrlsInText_data();
    void fileInfoUrlsInText();
    void fileExporterXMLsave_data();
    void fileExporterXMLsave();
    QHash<QString, QHash<const char *, QSet<QString>>> fileExporterXSLTtestCases();
    void fileExporterXSLTsaveFile_data();
    void fileExporterXSLTsaveFile();
    void fileExporterXSLTsaveElement_data();
    void fileExporterXSLTsaveElement();
    void fileExporterRISsave_data();
    void fileExporterRISsave();
    void fileExporterBibTeXsave_data();
    void fileExporterBibTeXsave();
    void fileImporterRISload_data();
    void fileImporterRISload();
    void fileImporterBibTeXload_data();
    void fileImporterBibTeXload();
    void fileExporterBibTeXEncoding_data();
    void fileExporterBibTeXEncoding();
    void fileImportExportBibTeXroundtrip_data();
    void fileImportExportBibTeXroundtrip();
    void protectiveCasingEntryGeneratedOnTheFly();
    void protectiveCasingEntryFromData();
    void partialBibTeXInput_data();
    void partialBibTeXInput();
    void partialRISInput_data();
    void partialRISInput();

private:
};

void KBibTeXIOTest::encoderConvertToPlainAscii_data()
{
    QTest::addColumn<QString>("unicodestring");
    /// Depending on the chosen implementation for Encoder::instance().convertToPlainAscii(),
    /// the ASCII variant may slightly differ (both alternatives are considered valid).
    /// If both implementations produce the same ASCII output, 'asciialternative2' is
    /// to be set to be empty.
    QTest::addColumn<QString>("asciialternative1");
    QTest::addColumn<QString>("asciialternative2");

    QTest::newRow("Just 'A'") << QString(QChar(0x00c0)) + QChar(0x00c2) + QChar(0x00c5) << QStringLiteral("AAA") << QString();
    QTest::newRow("Just ASCII letters and numbers") << QStringLiteral("qwertyuiopASDFGHJKLzxcvbnm1234567890") << QStringLiteral("qwertyuiopASDFGHJKLzxcvbnm1234567890") << QString();
    QTest::newRow("Latin text") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QString();
    QTest::newRow("ASCII low and high bytes") << QStringLiteral("\x00\x01\x09\x0a\x10\x11\x19\x1a\x1f\x20\x7e\x7f") << QStringLiteral(" ~") << QString();
    QTest::newRow("European Scripts/Latin-1 Supplement") << QString::fromUtf8("\xc3\x80\xc3\x82\xc3\x84\xc3\x92\xc3\x94\xc3\x96\xc3\xac\xc3\xad\xc3\xae\xc3\xaf") << QStringLiteral("AAAOOOiiii") << QStringLiteral("AAAEOOOEiiii");
    QTest::newRow("European Scripts/Latin Extended-A") << QString::fromUtf8("\xc4\x8a\xc4\x8b\xc4\xae\xc4\xaf\xc5\x9c\xc5\x9d\xc5\xbb\xc5\xbc") << QStringLiteral("CcIiSsZz") << QString();
    QTest::newRow("European Scripts/Latin Extended-B") << QString::fromUtf8("\xc7\x8a\xc7\x8b\xc7\x8c") << QStringLiteral("NJNjnj") << QString();
    QTest::newRow("European Scripts/Latin Extended Additional") << QString::fromUtf8("\xe1\xb8\xbe\xe1\xb8\xbf\xe1\xb9\xa4\xe1\xb9\xa5\xe1\xbb\xae\xe1\xbb\xaf") << QStringLiteral("MmSsUu") << QString();
    QTest::newRow("European Scripts/Cyrillic") << QString::fromUtf8("\xd0\x90\xd0\x9e\xd0\x9f") << QStringLiteral("AOP") << QString();
    QTest::newRow("European Scripts/Greek and Coptic") << QString::fromUtf8("\xce\xba\xce\xb1\xce\xa4\xcf\xba\xce\x9d") << QStringLiteral("kaTSN") << QStringLiteral("kappaalphaTauSanNu");
    QTest::newRow("East Asian Scripts/Katakana") << QString::fromUtf8("\xe3\x82\xb7\xe3\x83\x84") << QStringLiteral("shitsu") << QStringLiteral("situ");
    QTest::newRow("East Asian Scripts/Hangul Syllables") << QString::fromUtf8("\xea\xb9\x80\xec\xa0\x95\xec\x9d\x80") << QStringLiteral("gimjeongeun") << QStringLiteral("gimjeong-eun");
    QTest::newRow("Non-BMP characters (stay unchanged)") << QString::fromUtf8(/* U+10437 */ "\xf0\x90\x90\xb7" /* U+10E6D */ "\xf0\x90\xb9\xad" /* U+1D11E */ "\xf0\x9d\x84\x9e" /* U+10FFFF */ "") << QString::fromUtf8("\xf0\x90\x90\xb7\xf0\x90\xb9\xad\xf0\x9d\x84\x9e") << QString();
    QTest::newRow("Base symbols followed by combining symbols") << QString::fromUtf8("123" /* COMBINING GRAVE ACCENT */ "A\xcc\x80" /* COMBINING DIAERESIS */ "A\xcc\x88" /* COMBINING LOW LINE */ "A\xcc\xb2" "123") << QStringLiteral("123AAA123") << QString();
}

void KBibTeXIOTest::encoderConvertToPlainAscii()
{
    QFETCH(QString, unicodestring);
    QFETCH(QString, asciialternative1);
    QFETCH(QString, asciialternative2);

    const QString converted = Encoder::instance().convertToPlainAscii(unicodestring);
    /// Depending on the chosen implementation for Encoder::instance().convertToPlainAscii(),
    /// the ASCII variant may slightly differ (both alternatives are considered valid).
    if (converted != asciialternative1 && converted != asciialternative2)
        qCWarning(LOG_KBIBTEX_TEST) << "converted=" << converted << "  asciialternative1=" << asciialternative1 << "  asciialternative2=" << asciialternative2;
    QVERIFY(converted == asciialternative1 || converted == asciialternative2);
}

void KBibTeXIOTest::encoderXMLdecode_data()
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QString>("unicode");

    QTest::newRow("Just ASCII") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.");
    QTest::newRow("Quotation marks") << QStringLiteral("Caesar said: &quot;Veni, vidi, vici&quot;") << QStringLiteral("Caesar said: \"Veni, vidi, vici\"");
    QTest::newRow("Characters from EncoderXMLCharMapping") << QStringLiteral("&quot;&amp;&lt;&gt;") << QStringLiteral("\"\\&<>");
    QTest::newRow("Characters from backslashSymbols") << QStringLiteral("&amp;%_") << QStringLiteral("\\&\\%\\_");

    for (int start = 0; start < 16; ++start) {
        QString xmlString, unicodeString;
        for (int offset = 1561; offset < 6791; offset += 621) {
            const ushort unicode = static_cast<ushort>((start * 3671 + offset) & 0x7fff);
            xmlString += QStringLiteral("&#") + QString::number(unicode) + QStringLiteral(";");
            unicodeString += QChar(unicode);
        }
        QTest::newRow(QString(QStringLiteral("Some arbitrary Unicode characters (%1): %2")).arg(start).arg(xmlString).toLatin1().constData()) << xmlString << unicodeString;
    }
}

void KBibTeXIOTest::encoderXMLdecode()
{
    QFETCH(QString, xml);
    QFETCH(QString, unicode);

    QCOMPARE(EncoderXML::instance().decode(xml), unicode);
}

void KBibTeXIOTest::encoderXMLencode_data()
{
    encoderXMLdecode_data();
}

void KBibTeXIOTest::encoderXMLencode()
{
    QFETCH(QString, xml);
    QFETCH(QString, unicode);

    QCOMPARE(EncoderXML::instance().encode(unicode, Encoder::TargetEncoding::ASCII), xml);
}

void KBibTeXIOTest::encoderLaTeXdecode_data()
{
    QTest::addColumn<QString>("latex");
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QString>("alternativelatex");

    QTest::newRow("Just ASCII") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QString();
    QTest::newRow("Dotless i and j characters") << QStringLiteral("{\\`\\i}{\\`{\\i}}{\\'\\i}{\\^\\i}{\\\"\\i}{\\~\\i}{\\=\\i}{\\u\\i}{\\k\\i}{\\^\\j}{\\m\\i}{\\v\\i}{\\v\\j}\\m\\i") << QString(QChar(0x00EC)) + QChar(0x00EC) + QChar(0x00ED) + QChar(0x00EE) + QChar(0x00EF) + QChar(0x0129) + QChar(0x012B) + QChar(0x012D) + QChar(0x012F) + QChar(0x0135) + QStringLiteral("{\\m\\i}")  + QChar(0x01D0) + QChar(0x01F0) + QStringLiteral("\\m\\i") <<  QStringLiteral("{\\`\\i}{\\`\\i}{\\'\\i}{\\^\\i}{\\\"\\i}{\\~\\i}{\\=\\i}{\\u\\i}{\\k\\i}{\\^\\j}{\\m\\i}{\\v\\i}{\\v\\j}\\m\\i");
    QTest::newRow("\\l and \\ldots") << QStringLiteral("\\l\\ldots\\l\\ldots") << QString(QChar(0x0142)) + QChar(0x2026) + QChar(0x0142) + QChar(0x2026) << QStringLiteral("{\\l}{\\ldots}{\\l}{\\ldots}");
    QTest::newRow("Various two-letter commands (1)") << QStringLiteral("\\AA\\textmugreek") << QString(QChar(0x00c5)) + QChar(0x03bc) << QStringLiteral("{\\AA}{\\textmugreek}");
    QTest::newRow("Various two-letter commands (2)") << QStringLiteral("{\\AA}{\\textmugreek}") << QString(QChar(0x00c5)) + QChar(0x03bc) << QStringLiteral("{\\AA}{\\textmugreek}");
    QTest::newRow("Various two-letter commands (3)") << QStringLiteral("\\AA \\textmugreek") << QString(QChar(0x00c5)) + QChar(0x03bc) << QStringLiteral("{\\AA}{\\textmugreek}");
    QTest::newRow("Inside curly brackets: modifier plus letter") << QStringLiteral("aa{\\\"A}bb{\\\"T}") << QStringLiteral("aa") + QChar(0x00c4) + QStringLiteral("bb{\\\"T}") << QString();
    QTest::newRow("Inside curly brackets: modifier plus, inside curly brackets, letter") << QStringLiteral("aa{\\\"{A}}bb{\\\"{T}}") << QStringLiteral("aa") + QChar(0x00c4) + QStringLiteral("bb{\\\"{T}}") <<  QStringLiteral("aa{\\\"A}bb{\\\"{T}}");
    QTest::newRow("Modifier plus letter") << QStringLiteral("\\\"A aa\\\"Abb\\\"T") << QChar(0x00c4) + QStringLiteral(" aa") + QChar(0x00c4) + QStringLiteral("bb\\\"T") << QStringLiteral("{\\\"A} aa{\\\"A}bb\\\"T");
    QTest::newRow("Modifier plus, inside curly brackets, letter") << QStringLiteral("\\\"{A} aa\\\"{A}bb\\\"{T}") << QChar(0x00c4) + QStringLiteral(" aa") + QChar(0x00c4) + QStringLiteral("bb\\\"{T}") << QStringLiteral("{\\\"A} aa{\\\"A}bb\\\"{T}");
    QTest::newRow("Single-letter commands") << QStringLiteral("\\,\\&\\_\\#\\%") << QChar(0x2009) + QStringLiteral("&_#%") << QString();
    QTest::newRow("\\noopsort{\\noopsort}") << QStringLiteral("\\noopsort{\\noopsort}") << QStringLiteral("\\noopsort{\\noopsort}") << QString();
    QTest::newRow("\\ensuremath") << QStringLiteral("\\ensuremath{${\\alpha}$}${\\ensuremath{\\delta}}$-spot \\ensuremath{26^{\\mathrm{th}}} {\\ensuremath{-}}") << QStringLiteral("\\ensuremath{$") + QChar(0x03b1) + ("$}${\\ensuremath{") + QChar(0x03b4) + QStringLiteral("}}$-spot \\ensuremath{26^{\\mathrm{th}}} {\\ensuremath{-}}") << QStringLiteral("\\ensuremath{$\\alpha$}${\\ensuremath{\\delta}}$-spot \\ensuremath{26^{\\mathrm{th}}} {\\ensuremath{-}}");
    QTest::newRow("Greek mu with 'Dollar' math") << QString(QStringLiteral("%1\\mu\\textmu$%1\\mu$")).arg(QChar(0x03bc)) << QString(QStringLiteral("%1%1%1$%1%1$")).arg(QChar(0x03bc)) << QStringLiteral("{\\textmugreek}{\\textmugreek}{\\textmugreek}$\\mu{}\\mu$");
    QTest::newRow("Greek mu with '\\ensuremath'") << QString(QStringLiteral("%1\\mu\\textmu\\ensuremath{%1\\mu}")).arg(QChar(0x03bc)) << QString(QStringLiteral("%1%1%1\\ensuremath{%1%1}")).arg(QChar(0x03bc)) << QString(QStringLiteral("{\\textmugreek}{\\textmugreek}{\\textmugreek}\\ensuremath{\\mu{}\\mu}"));
}

void KBibTeXIOTest::encoderLaTeXdecode()
{
    QFETCH(QString, latex);
    QFETCH(QString, unicode);

    QCOMPARE(EncoderLaTeX::instance().decode(latex), unicode);
}

void KBibTeXIOTest::encoderLaTeXencode_data()
{
    encoderLaTeXdecode_data();
}

void KBibTeXIOTest::encoderLaTeXencode()
{
    QFETCH(QString, latex);
    QFETCH(QString, unicode);
    QFETCH(QString, alternativelatex);

    const QString generatedLatex = EncoderLaTeX::instance().encode(unicode, Encoder::TargetEncoding::ASCII);
    if (generatedLatex != latex && !alternativelatex.isEmpty())
        QCOMPARE(generatedLatex, alternativelatex);
    else
        QCOMPARE(generatedLatex, latex);
}

void KBibTeXIOTest::encoderLaTeXencodeHash()
{
    /// File with single entry, inspired by 'Moby Dick'
    QScopedPointer<File> file(new File());
    QSharedPointer<Entry> entry(new Entry(Entry::etBook, QStringLiteral("latex-encoder-test")));
    file->append(entry);
    static const QString titleText(QStringLiteral("{This is a Title with a \\#}"));
    entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(titleText)));
    static const QString authorLastNameText(QStringLiteral("{Flash the \\#}"));
    entry->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QString(), authorLastNameText)));
    static const QString urlText(QStringLiteral("https://127.0.0.1/#hashtag"));
    entry->insert(Entry::ftUrl, Value() << QSharedPointer<VerbatimText>(new VerbatimText(urlText)));
    file->setProperty(File::ProtectCasing, Qt::Checked);
    file->setProperty(File::Encoding, QStringLiteral("latex"));

    QBuffer fileBuffer;
    fileBuffer.open(QBuffer::WriteOnly);
    FileExporterBibTeX exporter(this);
    exporter.save(&fileBuffer, file.data());
    fileBuffer.close();

    fileBuffer.open(QBuffer::ReadOnly);
    FileImporterBibTeX importer(this);
    QScopedPointer<File> importedFile(importer.load(&fileBuffer));
    fileBuffer.close();

    QVERIFY(!importedFile.isNull());
    QVERIFY(importedFile->count() == 1);
    QSharedPointer<Entry> importedEntry = importedFile->first().dynamicCast<Entry>();
    QVERIFY(!importedEntry.isNull());
    QVERIFY(importedEntry->count() == 3);
    QVERIFY(importedEntry->contains(Entry::ftTitle));
    QVERIFY(importedEntry->value(Entry::ftTitle).count() == 1);
    const QSharedPointer<PlainText> titlePlainText = importedEntry->value(Entry::ftTitle).first().dynamicCast<PlainText>();
    QVERIFY(!titlePlainText.isNull());
    const QString importedTitleText = titlePlainText->text();
    QVERIFY(!importedTitleText.isEmpty());
    QVERIFY(importedEntry->contains(Entry::ftAuthor));
    QVERIFY(importedEntry->value(Entry::ftAuthor).count() == 1);
    const QSharedPointer<Person> authorPerson = importedEntry->value(Entry::ftAuthor).first().dynamicCast<Person>();
    QVERIFY(!authorPerson.isNull());
    const QString importedAuthorLastNameText = authorPerson->lastName();
    QVERIFY(!importedAuthorLastNameText.isEmpty());
    QVERIFY(importedEntry->contains(Entry::ftUrl));
    QVERIFY(importedEntry->value(Entry::ftUrl).count() == 1);
    const QSharedPointer<VerbatimText> urlVerbatimText = importedEntry->value(Entry::ftUrl).first().dynamicCast<VerbatimText>();
    QVERIFY(!urlVerbatimText.isNull());
    const QString importedUrlText = urlVerbatimText->text();
    QVERIFY(!importedUrlText.isEmpty());

    QVERIFY(importedTitleText == titleText);
    QVERIFY(importedAuthorLastNameText == authorLastNameText);
    QVERIFY(importedUrlText == urlText);
}

void KBibTeXIOTest::fileImporterSplitName_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<Person *>("person");

    QTest::newRow("Empty name") << QString() << new Person(QString(), QString(), QString());
    QTest::newRow("PubMed style") << QStringLiteral("Jones A B C") << new Person(QStringLiteral("A B C"), QStringLiteral("Jones"), QString());
    QTest::newRow("Just last name") << QStringLiteral("Dido") << new Person(QString(), QStringLiteral("Dido"), QString());
    QTest::newRow("Name with 'von'") << QStringLiteral("Theodor von Sickel") << new Person(QStringLiteral("Theodor"), QStringLiteral("von Sickel"), QString());
    QTest::newRow("Name with 'von', reversed") << QStringLiteral("von Sickel, Theodor") << new Person(QStringLiteral("Theodor"), QStringLiteral("von Sickel"), QString());
    QTest::newRow("Name with 'van der'") << QStringLiteral("Adriaen van der Werff") << new Person(QStringLiteral("Adriaen"), QStringLiteral("van der Werff"), QString());
    QTest::newRow("Name with 'van der', reversed") << QStringLiteral("van der Werff, Adriaen") << new Person(QStringLiteral("Adriaen"), QStringLiteral("van der Werff"), QString());
    QTest::newRow("Name with suffix") << QStringLiteral("Anna Eleanor Roosevelt Jr.") << new Person(QStringLiteral("Anna Eleanor"), QStringLiteral("Roosevelt"), QStringLiteral("Jr."));
}

void KBibTeXIOTest::fileImporterSplitName()
{
    QFETCH(QString, name);
    QFETCH(Person *, person);

    Person *computedPerson = FileImporter::splitName(name);
    QCOMPARE(*computedPerson, *person);

    delete person;
    delete computedPerson;
}

void KBibTeXIOTest::fileInfoMimeTypeForUrl_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QMimeType>("mimetype");

    static const QMimeDatabase db;
    QTest::newRow("Invalid URL") << QUrl() << QMimeType();
    QTest::newRow("Generic URL") << QUrl(QStringLiteral("https://www.example.com")) << db.mimeTypeForName(QStringLiteral("text/html"));
    QTest::newRow("Generic local file") << QUrl(QStringLiteral("/usr/bin/who")) << db.mimeTypeForName(QStringLiteral("application/octet-stream"));
    QTest::newRow("Generic Samba URL") << QUrl(QStringLiteral("smb://fileserver.local/file")) << db.mimeTypeForName(QStringLiteral("application/octet-stream"));
    QTest::newRow("URL to .bib file") << QUrl(QStringLiteral("https://www.example.com/references.bib")) << db.mimeTypeForName(QStringLiteral("text/x-bibtex"));
    QTest::newRow("Local .bib file") << QUrl(QStringLiteral("/home/user/references.bib")) << db.mimeTypeForName(QStringLiteral("text/x-bibtex"));
    QTest::newRow("URL to .pdf file") << QUrl(QStringLiteral("https://www.example.com/references.pdf")) << db.mimeTypeForName(QStringLiteral("application/pdf"));
    QTest::newRow("Local .pdf file") << QUrl(QStringLiteral("/home/user/references.pdf")) << db.mimeTypeForName(QStringLiteral("application/pdf"));
}

void KBibTeXIOTest::fileInfoMimeTypeForUrl()
{
    QFETCH(QUrl, url);
    QFETCH(QMimeType, mimetype);

    QCOMPARE(FileInfo::mimeTypeForUrl(url), mimetype);
}

void KBibTeXIOTest::fileInfoUrlsInText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QSet<QUrl>>("expectedUrls");

    QTest::newRow("Empty text") << QString() << QSet<QUrl>();
    QTest::newRow("Lore ipsum with DOI (without URL)") << QStringLiteral("Lore ipsum 10.1000/38-abc Lore ipsum") << QSet<QUrl> {QUrl(KBibTeX::doiUrlPrefix + QStringLiteral("10.1000/38-abc"))};
    QTest::newRow("Lore ipsum with DOI (with HTTP URL)") << QStringLiteral("Lore ipsum http://doi.example.org/10.1000/38-abc Lore ipsum") << QSet<QUrl> {QUrl(KBibTeX::doiUrlPrefix + QStringLiteral("10.1000/38-abc"))};
    QTest::newRow("Lore ipsum with DOI (with HTTPS URL)") << QStringLiteral("Lore ipsum https://doi.example.org/10.1000/42-XYZ Lore ipsum") << QSet<QUrl> {QUrl(KBibTeX::doiUrlPrefix + QStringLiteral("10.1000/42-XYZ"))};
    QTest::newRow("URLs and DOI (without URL), all semicolon-separated") << QStringLiteral("http://www.example.com;10.1000/38-abc   ;\nhttps://www.example.com") << QSet<QUrl> {QUrl(QStringLiteral("http://www.example.com")), QUrl(KBibTeX::doiUrlPrefix + QStringLiteral("10.1000/38-abc")), QUrl(QStringLiteral("https://www.example.com"))};
    QTest::newRow("URLs and DOI (with URL), all semicolon-separated") << QStringLiteral("http://www.example.com\n;   10.1000/38-abc;https://www.example.com") << QSet<QUrl> {QUrl(QStringLiteral("http://www.example.com")), QUrl(KBibTeX::doiUrlPrefix + QStringLiteral("10.1000/38-abc")), QUrl(QStringLiteral("https://www.example.com"))};
    QTest::newRow("URLs with various separators") << QStringLiteral("http://www.example.com/def.pdf https://www.example.com\nhttp://download.example.com/abc") << QSet<QUrl> {QUrl(QStringLiteral("http://www.example.com/def.pdf")), QUrl(QStringLiteral("https://www.example.com")), QUrl(QStringLiteral("http://download.example.com/abc"))};
    QTest::newRow("URLs with query strings and anchors") << QStringLiteral("http://www.example.com/def.pdf?a=3&b=1 https://www.example.com#1581584\nhttp://download.example.com/abc,7352,A#abc?gh=352&ghi=1254") << QSet<QUrl> {QUrl(QStringLiteral("http://www.example.com/def.pdf?a=3&b=1")), QUrl(QStringLiteral("https://www.example.com#1581584")), QUrl(QStringLiteral("http://download.example.com/abc,7352,A#abc?gh=352&ghi=1254"))};
}

void KBibTeXIOTest::fileInfoUrlsInText()
{
    QFETCH(QString, text);
    QFETCH(QSet<QUrl>, expectedUrls);

    QSet<QUrl> extractedUrls;
    FileInfo::urlsInText(text, FileInfo::TestExistence::No, QString(), extractedUrls);

    QCOMPARE(extractedUrls.count(), expectedUrls.count());
    for (const QUrl &expectedUrl : const_cast<const QSet<QUrl> &>(expectedUrls))
        QCOMPARE(extractedUrls.contains(expectedUrl), true);
}

static const char *fileImporterExporterTestCases_Label_Empty_file = "Empty file";
static const char *fileImporterExporterTestCases_Label_Moby_Dick = "Moby Dick";

File *KBibTeXIOTest::mobyDickBibliography()
{
    static File *mobyDickFile = nullptr;
    if (mobyDickFile == nullptr) {
        /// File with single entry, inspired by 'Moby Dick'
        mobyDickFile = new File();
        QSharedPointer<Entry> entry(new Entry(Entry::etBook, QStringLiteral("the-whale-1851")));
        mobyDickFile->append(entry);
        entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Call me Ishmael}"))));
        entry->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Herman"), QStringLiteral("Melville"))) << QSharedPointer<Person>(new Person(QStringLiteral("Moby"), QStringLiteral("Dick"))));
        entry->insert(Entry::ftMonth, Value() << QSharedPointer<MacroKey>(new MacroKey(QStringLiteral("jun"))));
        entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("1851"))));
        mobyDickFile->setProperty(File::ProtectCasing, Qt::Checked);
    }
    return mobyDickFile;
}

File *KBibTeXIOTest::latinUmlautBibliography()
{
    static File *latinUmlautFile = nullptr;
    if (latinUmlautFile == nullptr) {
        latinUmlautFile = new File();
        QSharedPointer<Entry> entry(new Entry(Entry::etArticle, QStringLiteral("einstein1907relativitaetsprinzip")));
        latinUmlautFile->append(entry);
        entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QString(QStringLiteral("{%1ber das Relativit%2tsprinzip und die aus demselben gezogenen Folgerungen}")).arg(QChar(0x00DC)).arg(QChar(0x00E4)))));
        entry->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Albert"), QStringLiteral("Einstein"))));
        entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("1907/08"))));
        entry->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("23") + QChar(0x2013) + QStringLiteral("42"))));
        latinUmlautFile->setProperty(File::ProtectCasing, Qt::Checked);
    }
    return latinUmlautFile;
}

File *KBibTeXIOTest::koreanBibliography()
{
    static File *koreanFile = nullptr;
    if (koreanFile == nullptr) {
        koreanFile = new File();
        QSharedPointer<Entry> entry(new Entry(Entry::etMastersThesis, QStringLiteral("korean-text-copied-from-dailynk")));
        koreanFile->append(entry);
        entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QString::fromUtf8("\xeb\xb3\xb4\xec\x9c\x84\xea\xb5\xad \xec\x88\x98\xec\x82\xac\xeb\xb6\x80\xc2\xb7\xec\x98\x81\xec\xb0\xbd\xea\xb4\x80\xeb\xa6\xac\xeb\xb6\x80 \xec\x8a\xb9\xea\xb2\xa9\xe2\x80\xa6\xea\xb9\x80\xec\xa0\x95\xec\x9d\x80, \xeb\xb6\x80\xec\xa0\x95\xeb\xb6\x80\xed\x8c\xa8\xec\x99\x80 \xec\xa0\x84\xec\x9f\x81 \xec\x98\x88\xea\xb3\xa0"))));
        entry->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QString::fromUtf8("\xec\xa0\x95\xec\x9d\x80"), QString::fromUtf8("\xea\xb9\x80"))));
        entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("1921"))));
        koreanFile->setProperty(File::NameFormatting, Preferences::personNameFormatLastFirst);
        koreanFile->setProperty(File::ProtectCasing, Qt::Unchecked);
    }
    return koreanFile;
}

File *KBibTeXIOTest::russianBibliography()
{
    static File *russianFile = nullptr;
    if (russianFile == nullptr) {
        russianFile = new File();
        QSharedPointer<Entry> entry(new Entry(Entry::etBook, QStringLiteral("war-and-peace")));
        russianFile->append(entry);
        entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QString::fromUtf8("\xd0\x92\xd0\xbe\xd0\xb9\xd0\xbd\xd0\xb0 \xd0\xb8 \xd0\xbc\xd0\xb8\xd1\x80"))));
        entry->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QString::fromUtf8("\xd0\x9b\xd0\xb5\xd0\xb2"), QString::fromUtf8("{\xd0\x9d\xd0\xb8\xd0\xba\xd0\xbe\xd0\xbb\xd0\xb0\xd0\xb5\xd0\xb2\xd0\xb8\xd1\x87 \xd0\xa2\xd0\xbe\xd0\xbb\xd1\x81\xd1\x82\xd0\xbe\xd0\xb9}"))));
        entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("1869"))));
        russianFile->setProperty(File::NameFormatting, Preferences::personNameFormatLastFirst);
        russianFile->setProperty(File::ProtectCasing, Qt::Unchecked);
    }
    return russianFile;
}

QVector<QPair<const char *, File *> > KBibTeXIOTest::fileImporterExporterTestCases()
{
    /// The vector 'result' is static so that if this function is invoked multiple
    /// times, the vector will be initialized and filled with File objects only upon
    /// the function's first invocation.
    static QVector<QPair<const char *, File *> > result;

    if (result.isEmpty()) {
        /// Empty file without any entries
        result.append(QPair<const char *, File *>(fileImporterExporterTestCases_Label_Empty_file, new File()));

        /// File with single entry, inspired by 'Moby Dick'
        result.append(QPair<const char *, File *>(fileImporterExporterTestCases_Label_Moby_Dick, mobyDickBibliography()));

        // TODO add more file objects to result vector

        /// Set various properties to guarantee reproducible results irrespective of local settings
        for (auto it = result.constBegin(); it != result.constEnd(); ++it) {
            File *file = it->second;
            file->setProperty(File::NameFormatting, Preferences::personNameFormatLastFirst);
            file->setProperty(File::ProtectCasing, static_cast<int>(Qt::Checked));
            // TODO more file properties to set?
        }
    }

    return result;
}

void KBibTeXIOTest::fileExporterXMLsave_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("xmlData");

    static const QHash<const char *, QString> keyToXmlData {
        {fileImporterExporterTestCases_Label_Empty_file, QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>|<!-- XML document written by KBibTeXIO as part of KBibTeX -->|<!-- https://userbase.kde.org/KBibTeX -->|<bibliography>|</bibliography>|")},
        {fileImporterExporterTestCases_Label_Moby_Dick, QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>|<!-- XML document written by KBibTeXIO as part of KBibTeX -->|<!-- https://userbase.kde.org/KBibTeX -->|<bibliography>| <entry id=\"the-whale-1851\" type=\"book\">|  <authors>|<person><firstname>Herman</firstname><lastname>Melville</lastname></person><person><firstname>Moby</firstname><lastname>Dick</lastname></person>|  </authors>|  <month triple=\"jun\" number=\"6\"><text>June</text></month>|  <title><text>Call me Ishmael</text></title>|  <year number=\"1851\"><text>1851</text></year>| </entry>|</bibliography>|")}
    };
    static const QVector<QPair<const char *, File *> > keyFileTable = fileImporterExporterTestCases();

    for (auto it = keyFileTable.constBegin(); it != keyFileTable.constEnd(); ++it)
        if (keyToXmlData.contains(it->first))
            QTest::newRow(it->first) << it->second << keyToXmlData.value(it->first);
}

void KBibTeXIOTest::fileExporterXMLsave()
{
    QFETCH(File *, bibTeXfile);
    QFETCH(QString, xmlData);

    FileExporterXML fileExporterXML(this);
    const QString generatedData = fileExporterXML.toString(bibTeXfile).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|'));

    QCOMPARE(generatedData, xmlData);
}

QHash<QString, QHash<const char *, QSet<QString>>> KBibTeXIOTest::fileExporterXSLTtestCases()
{
    static const QHash<QString, QHash<const char *, QSet<QString>>> xsltTokeyToXsltData {
        {
            QStringLiteral("kbibtex/standard.xsl"),
            {
                {fileImporterExporterTestCases_Label_Empty_file, {QStringLiteral("<title>Bibliography</title>"), QStringLiteral("<body>|</body>")}},
                {fileImporterExporterTestCases_Label_Moby_Dick, {QStringLiteral("<title>Bibliography</title>"), QStringLiteral(">1851<"), QStringLiteral(">Call me Ishmael<"), QStringLiteral("</b>"), QStringLiteral("</body>")}}
            }
        },
        {
            QStringLiteral("kbibtex/fancy.xsl"),
            {
                {fileImporterExporterTestCases_Label_Empty_file, {QStringLiteral("<title>Bibliography</title>"), QStringLiteral("<body style=\"margin:0px; padding: 0px;\">|</body>")}},
                {fileImporterExporterTestCases_Label_Moby_Dick, {QStringLiteral("<title>Bibliography</title>"), QStringLiteral(" style=\"text-decoration: "), QStringLiteral("<span style=\""), QStringLiteral("ref=\"kbibtex:filter:year=1851\">1851</a>"), QStringLiteral("ef=\"kbibtex:filter:title=Call me Ishmael\">Call me Ishmael<"), QStringLiteral("ef=\"kbibtex:filter:author=Melville\">Melville, H.</a>")}}
            }
        },
        {
            QStringLiteral("kbibtex/wikipedia-cite.xsl"),
            {
                {fileImporterExporterTestCases_Label_Empty_file, {QStringLiteral("|")}},
                {fileImporterExporterTestCases_Label_Moby_Dick, {QStringLiteral("{cite book|"), QStringLiteral("|title=Call me Ishmael|"), QStringLiteral("|last1=Melville"), QStringLiteral("|first2=Moby"), QStringLiteral("|year=1851|")}}
            }
        }
    };
    return xsltTokeyToXsltData;
}

void KBibTeXIOTest::fileExporterXSLTsaveFile_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("xslTranslationFile");
    QTest::addColumn<QSet<QString>>("expectedFragments");

    static const auto &xsltToKeyToXsltData = fileExporterXSLTtestCases();
    static const QVector<QPair<const char *, File *> > keyFileTable = fileImporterExporterTestCases();

    for (const QString &xslTranslationFile : xsltToKeyToXsltData.keys()) {
        const QHash<const char *, QSet<QString>> &keyToXsltData = xsltToKeyToXsltData.value(xslTranslationFile);
        for (auto it = keyFileTable.constBegin(); it != keyFileTable.constEnd(); ++it)
            if (keyToXsltData.contains(it->first)) {
                const QString label = QString(QStringLiteral("'%1' with XSLT '%2'")).arg(QString::fromLatin1(it->first)).arg(xslTranslationFile);
                QTest::newRow(label.toUtf8().constData()) << it->second << xslTranslationFile << keyToXsltData.value(it->first);
            }
    }
}

void KBibTeXIOTest::fileExporterXSLTsaveFile()
{
    QFETCH(File *, bibTeXfile);
    QFETCH(QString, xslTranslationFile);
    QFETCH(QSet<QString>, expectedFragments);


    FileExporterXSLT fileExporterXSLT(QStandardPaths::locate(QStandardPaths::GenericDataLocation, xslTranslationFile), this);
    static const QRegularExpression removeSpaceBeforeTagRegExp(QStringLiteral("\\s+(<[/a-z])"));
    const QString generatedData = fileExporterXSLT.toString(bibTeXfile).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|')).replace(removeSpaceBeforeTagRegExp, QStringLiteral("\\1")).replace(QStringLiteral("|||"), QStringLiteral("|")).replace(QStringLiteral("||"), QStringLiteral("|"));

    for (const QString &fragment : expectedFragments)
        QVERIFY2(generatedData.contains(fragment), QString(QStringLiteral("Fragment '%1' not found in generated XML data")).arg(fragment).toLatin1().constData());
}

void KBibTeXIOTest::fileExporterXSLTsaveElement_data()
{
    QTest::addColumn<QSharedPointer<Element>>("element");
    QTest::addColumn<QString>("xslTranslationFile");
    QTest::addColumn<QSet<QString>>("expectedFragments");

    static const auto &xsltToKeyToXsltData = fileExporterXSLTtestCases();
    static const QVector<QPair<const char *, File *> > keyFileTable = fileImporterExporterTestCases();

    for (const QString &xslTranslationFile : xsltToKeyToXsltData.keys()) {
        const QHash<const char *, QSet<QString>> &keyToXsltData = xsltToKeyToXsltData.value(xslTranslationFile);
        for (auto it = keyFileTable.constBegin(); it != keyFileTable.constEnd(); ++it)
            if (!it->second->isEmpty() && keyToXsltData.contains(it->first)) {
                const QString label = QString(QStringLiteral("'%1' with XSLT '%2'")).arg(QString::fromLatin1(it->first)).arg(xslTranslationFile);
                QTest::newRow(label.toUtf8().constData()) << it->second->first() << xslTranslationFile << keyToXsltData.value(it->first);
            }
    }
}

void KBibTeXIOTest::fileExporterXSLTsaveElement()
{
    QFETCH(QSharedPointer<Element>, element);
    QFETCH(QString, xslTranslationFile);
    QFETCH(QSet<QString>, expectedFragments);

    FileExporterXSLT fileExporterXSLT(QStandardPaths::locate(QStandardPaths::GenericDataLocation, xslTranslationFile), this);
    static const QRegularExpression removeSpaceBeforeTagRegExp(QStringLiteral("\\s+(<[/a-z])"));
    const QString generatedData = fileExporterXSLT.toString(element, nullptr).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|')).replace(removeSpaceBeforeTagRegExp, QStringLiteral("\\1")).replace(QStringLiteral("|||"), QStringLiteral("|")).replace(QStringLiteral("||"), QStringLiteral("|"));

    for (const QString &fragment : expectedFragments)
        QVERIFY2(generatedData.contains(fragment), QString(QStringLiteral("Fragment '%1' not found in generated XML data")).arg(fragment).toLatin1().constData());
}

void KBibTeXIOTest::fileExporterRISsave_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("risData");

    static const QHash<const char *, QString> keyToRisData {
        {fileImporterExporterTestCases_Label_Empty_file, QString()},
        {fileImporterExporterTestCases_Label_Moby_Dick, QStringLiteral("TY  - BOOK|ID  - the-whale-1851|AU  - Melville, Herman|AU  - Dick, Moby|TI  - Call me Ishmael|PY  - 1851/06//|ER  - ||")}
    };
    static const QVector<QPair<const char *, File *> > keyFileTable = fileImporterExporterTestCases();

    for (auto it = keyFileTable.constBegin(); it != keyFileTable.constEnd(); ++it)
        if (keyToRisData.contains(it->first))
            QTest::newRow(it->first) << it->second << keyToRisData.value(it->first);
}

void KBibTeXIOTest::fileExporterRISsave()
{
    QFETCH(File *, bibTeXfile);
    QFETCH(QString, risData);

    FileExporterRIS fileExporterRIS(this);
    const QString generatedData = fileExporterRIS.toString(bibTeXfile).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|'));

    QCOMPARE(generatedData, risData);
}

void KBibTeXIOTest::fileExporterBibTeXsave_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("bibTeXdata");

    static const QHash<const char *, QString> keyToBibTeXData {
        {fileImporterExporterTestCases_Label_Empty_file, QString()},
        {fileImporterExporterTestCases_Label_Moby_Dick, QStringLiteral("@book{the-whale-1851,|\tauthor = {Melville, Herman and Dick, Moby},|\tmonth = jun,|\ttitle = {{Call me Ishmael}},|\tyear = {1851}|}||")}
    };
    static const QVector<QPair<const char *, File *> > keyFileTable = fileImporterExporterTestCases();

    for (auto it = keyFileTable.constBegin(); it != keyFileTable.constEnd(); ++it)
        if (keyToBibTeXData.contains(it->first))
            QTest::newRow(it->first) << it->second << keyToBibTeXData.value(it->first);
}

void KBibTeXIOTest::fileExporterBibTeXsave()
{
    QFETCH(File *, bibTeXfile);
    QFETCH(QString, bibTeXdata);

    FileExporterBibTeX fileExporterBibTeX(this);
    const QString generatedData = fileExporterBibTeX.toString(bibTeXfile).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|'));

    QCOMPARE(generatedData, bibTeXdata);
}

void KBibTeXIOTest::fileImporterRISload_data()
{
    QTest::addColumn<QByteArray>("risData");
    QTest::addColumn<File *>("bibTeXfile");

    static const QHash<const char *, QString> keyToRisData {
        {fileImporterExporterTestCases_Label_Empty_file, QString()},
        {fileImporterExporterTestCases_Label_Moby_Dick, QStringLiteral("TY  - BOOK|ID  - the-whale-1851|AU  - Melville, Herman|AU  - Dick, Moby|TI  - Call me Ishmael|PY  - 1851/06//|ER  - ||")}
    };
    static const QVector<QPair<const char *, File *> > keyFileTable = fileImporterExporterTestCases();

    for (auto it = keyFileTable.constBegin(); it != keyFileTable.constEnd(); ++it)
        if (keyToRisData.contains(it->first))
            QTest::newRow(it->first) << keyToRisData.value(it->first).toUtf8().replace('|', '\n') << it->second;
}

void KBibTeXIOTest::fileImporterRISload()
{
    QFETCH(QByteArray, risData);
    QFETCH(File *, bibTeXfile);

    FileImporterRIS fileImporterRIS(this);
    fileImporterRIS.setProtectCasing(true);
    QBuffer buffer(&risData);
    buffer.open(QBuffer::ReadOnly);
    QScopedPointer<File> generatedFile(fileImporterRIS.load(&buffer));

    QVERIFY(generatedFile->operator ==(*bibTeXfile));
}

void KBibTeXIOTest::fileImporterBibTeXload_data()
{
    QTest::addColumn<QByteArray>("bibTeXdata");
    QTest::addColumn<File *>("bibTeXfile");

    static const QHash<const char *, QString> keyToBibTeXData {
        {fileImporterExporterTestCases_Label_Empty_file, QString()},
        {fileImporterExporterTestCases_Label_Moby_Dick, QStringLiteral("@book{the-whale-1851,|\tauthor = {Melville, Herman and Dick, Moby},|\tmonth = jun,|\ttitle = {{Call me Ishmael}},|\tyear = {1851}|}||")}
    };
    static const QVector<QPair<const char *, File *> > keyFileTable = fileImporterExporterTestCases();

    for (auto it = keyFileTable.constBegin(); it != keyFileTable.constEnd(); ++it)
        if (keyToBibTeXData.contains(it->first))
            QTest::newRow(it->first) << keyToBibTeXData.value(it->first).toUtf8().replace('|', '\n') << it->second ;
}

void KBibTeXIOTest::fileImporterBibTeXload()
{
    QFETCH(QByteArray, bibTeXdata);
    QFETCH(File *, bibTeXfile);

    FileImporterBibTeX fileImporterBibTeX(this);
    QBuffer buffer(&bibTeXdata);
    buffer.open(QBuffer::ReadOnly);
    QScopedPointer<File> generatedFile(fileImporterBibTeX.load(&buffer));

    QVERIFY(generatedFile->operator ==(*bibTeXfile));
}

void KBibTeXIOTest::protectiveCasingEntryGeneratedOnTheFly()
{
    static const QString titleText = QStringLiteral("Some Title for a Journal Article");
    static const QString singleCurleyBracketTitle = QStringLiteral("{") + titleText + QStringLiteral("}");
    static const QString doubleCurleyBracketTitle = QStringLiteral("{{") + titleText + QStringLiteral("}}");

    FileExporterBibTeX fileExporterBibTeX(this);

    /// Create a simple File object with a title field
    File file;
    file.setProperty(File::StringDelimiter, QStringLiteral("{}"));
    QSharedPointer<Entry> entry {new Entry(Entry::etArticle, QStringLiteral("SomeId"))};
    Value titleValue = Value() << QSharedPointer<PlainText>(new PlainText(titleText));
    entry->insert(Entry::ftTitle, titleValue);
    file.append(entry);

    file.setProperty(File::ProtectCasing, Qt::Checked);
    const QString textWithProtectiveCasing = fileExporterBibTeX.toString(&file);
    QVERIFY(textWithProtectiveCasing.contains(doubleCurleyBracketTitle));

    file.setProperty(File::ProtectCasing, Qt::Unchecked);
    const QString textWithoutProtectiveCasing = fileExporterBibTeX.toString(&file);
    QVERIFY(textWithoutProtectiveCasing.contains(singleCurleyBracketTitle)
            && !textWithoutProtectiveCasing.contains(doubleCurleyBracketTitle));
}

void KBibTeXIOTest::fileExporterBibTeXEncoding_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("encoding");
    QTest::addColumn<QByteArray>("expectedOutput");

    static const QByteArray mobyDickEntryOutput {"@book{the-whale-1851,\n\tauthor = {Melville, Herman and Dick, Moby},\n\tmonth = jun,\n\ttitle = {{Call me Ishmael}},\n\tyear = {1851}\n}\n\n"};
    static const QStringList listOfASCIIcompatibleEncodings {QStringLiteral("LaTeX"), QStringLiteral("UTF-8"), QStringLiteral("ISO-8859-1"), QStringLiteral("Windows-1254"), QStringLiteral("ISO 2022-JP") /** technically not ASCII-compatible, but works for this case here */};
    for (const QString &encoding : listOfASCIIcompatibleEncodings) {
        const QByteArray encodingOutput{QByteArray{"@comment{x-kbibtex-encoding="} +encoding.toLatin1() + QByteArray{"}\n\n"}};
        const QByteArray mobyDickExpectedOutput{encoding == QStringLiteral("LaTeX") ? mobyDickEntryOutput : encodingOutput + mobyDickEntryOutput};
        QTest::newRow(QString(QStringLiteral("Moby Dick (ASCII only) encoded in '%1'")).arg(encoding).toLatin1().constData()) << mobyDickBibliography() << encoding << mobyDickExpectedOutput;
    }
    static const QByteArray mobyDickExpectedOutputUTF16 {"\xFF\xFE@\0""b\0o\0o\0k\0{\0t\0h\0""e\0-\0w\0h\0""a\0l\0""e\0-\0""1\0""8\0""5\0""1\0,\0\n\0\x09\0""a\0u\0t\0h\0o\0r\0 \0=\0 \0{\0M\0""e\0l\0v\0i\0l\0l\0""e\0,\0 \0H\0""e\0r\0m\0""a\0n\0 \0""a\0n\0""d\0 \0""D\0i\0""c\0k\0,\0 \0M\0o\0""b\0y\0}\0,\0\n\0\x09\0m\0o\0n\0t\0h\0 \0=\0 \0j\0u\0n\0,\0\n\0\x09\0t\0i\0t\0l\0""e\0 \0=\0 \0{\0{\0""C\0""a\0l\0l\0 \0m\0""e\0 \0I\0s\0h\0m\0""a\0""e\0l\0}\0}\0,\0\n\0\x09\0y\0""e\0""a\0r\0 \0=\0 \0{\0""1\0""8\0""5\0""1\0}\0\n\0}\0\n\0\n\0", 260};
    QTest::newRow("Moby Dick (ASCII only) encoded in 'UTF-16'") << mobyDickBibliography() << QStringLiteral("UTF-16") << mobyDickExpectedOutputUTF16;
    static const QByteArray mobyDickExpectedOutputUTF32 {"\xFF\xFE\0\0@\0\0\0""b\0\0\0o\0\0\0o\0\0\0k\0\0\0{\0\0\0t\0\0\0h\0\0\0""e\0\0\0-\0\0\0w\0\0\0h\0\0\0""a\0\0\0l\0\0\0""e\0\0\0-\0\0\0""1\0\0\0""8\0\0\0""5\0\0\0""1\0\0\0,\0\0\0\n\0\0\0\x09\0\0\0""a\0\0\0u\0\0\0t\0\0\0h\0\0\0o\0\0\0r\0\0\0 \0\0\0=\0\0\0 \0\0\0{\0\0\0M\0\0\0""e\0\0\0l\0\0\0v\0\0\0i\0\0\0l\0\0\0l\0\0\0""e\0\0\0,\0\0\0 \0\0\0H\0\0\0""e\0\0\0r\0\0\0m\0\0\0""a\0\0\0n\0\0\0 \0\0\0""a\0\0\0n\0\0\0""d\0\0\0 \0\0\0""D\0\0\0i\0\0\0""c\0\0\0k\0\0\0,\0\0\0 \0\0\0M\0\0\0o\0\0\0""b\0\0\0y\0\0\0}\0\0\0,\0\0\0\n\0\0\0\x09\0\0\0m\0\0\0o\0\0\0n\0\0\0t\0\0\0h\0\0\0 \0\0\0=\0\0\0 \0\0\0j\0\0\0u\0\0\0n\0\0\0,\0\0\0\n\0\0\0\x09\0\0\0t\0\0\0i\0\0\0t\0\0\0l\0\0\0""e\0\0\0 \0\0\0=\0\0\0 \0\0\0{\0\0\0{\0\0\0""C\0\0\0""a\0\0\0l\0\0\0l\0\0\0 \0\0\0m\0\0\0""e\0\0\0 \0\0\0I\0\0\0s\0\0\0h\0\0\0m\0\0\0""a\0\0\0""e\0\0\0l\0\0\0}\0\0\0}\0\0\0,\0\0\0\n\0\0\0\x09\0\0\0y\0\0\0""e\0\0\0""a\0\0\0r\0\0\0 \0\0\0=\0\0\0 \0\0\0{\0\0\0""1\0\0\0""8\0\0\0""5\0\0\0""1\0\0\0}\0\0\0\n\0\0\0}\0\0\0\n\0\0\0\n\0\0\0", 520};
    QTest::newRow("Moby Dick (ASCII only) encoded in 'UTF-32'") << mobyDickBibliography() << QStringLiteral("UTF-32") << mobyDickExpectedOutputUTF32;
    static const QByteArray latinUmlautExpectedOutputLaTeX {"@article{einstein1907relativitaetsprinzip,\n\tauthor = {Einstein, Albert},\n\tpages = {23--42},\n\ttitle = {{{\\\"U}ber das Relativit{\\\"a}tsprinzip und die aus demselben gezogenen Folgerungen}},\n\tyear = {1907/08}\n}\n\n"};
    QTest::newRow("Albert Einstein (latin umlaut) data encoded in 'LaTeX'") << latinUmlautBibliography() << QStringLiteral("LaTeX") << latinUmlautExpectedOutputLaTeX;
    static const QByteArray latinUmlautExpectedOutputUTF8 {"@comment{x-kbibtex-encoding=UTF-8}\n\n@article{einstein1907relativitaetsprinzip,\n\tauthor = {Einstein, Albert},\n\tpages = {23\xE2\x80\x93""42},\n\ttitle = {{\xC3\x9C""ber das Relativit\xC3\xA4tsprinzip und die aus demselben gezogenen Folgerungen}},\n\tyear = {1907/08}\n}\n\n"};
    QTest::newRow("Albert Einstein (latin umlaut) encoded in 'UTF-8'") << latinUmlautBibliography() << QStringLiteral("UTF-8") << latinUmlautExpectedOutputUTF8;
}

void KBibTeXIOTest::fileExporterBibTeXEncoding()
{
    QFETCH(File *, bibTeXfile);
    QFETCH(QString, encoding);
    QFETCH(QByteArray, expectedOutput);

    FileExporterBibTeX exporter(this);
    exporter.setEncoding(encoding);
    QByteArray generatedOutput;
    generatedOutput.reserve(8192);
    QBuffer buffer(&generatedOutput);
    buffer.open(QBuffer::WriteOnly);
    QVERIFY(exporter.save(&buffer, bibTeXfile));
    buffer.close();

    QCOMPARE(generatedOutput, expectedOutput);
}

void KBibTeXIOTest::fileImportExportBibTeXroundtrip_data() {
    struct TestCase {
        QString label;
        File *bibliography;
        QStringList encodingsToBeTested;
    };
    static const QVector<struct TestCase> testCases {
        {QStringLiteral("Moby Dick"), mobyDickBibliography(), Preferences::availableBibTeXEncodings},
        {QStringLiteral("Albert Einstein"), latinUmlautBibliography(), Preferences::availableBibTeXEncodings},
        {QStringLiteral("Kim Jong-un"), koreanBibliography(), {QStringLiteral("LaTeX"), QStringLiteral("UTF-8"), QStringLiteral("UTF-16"), QStringLiteral("UTF-16BE"), QStringLiteral("UTF-16LE"), QStringLiteral("UTF-32"), QStringLiteral("UTF-32BE"), QStringLiteral("UTF-32LE"), QStringLiteral("GB18030"), QStringLiteral("EUC-KR"), QStringLiteral("Windows-949")}},
        {QStringLiteral("L. Tolstoy"), russianBibliography(), {QStringLiteral("LaTeX"), QStringLiteral("ISO-8859-5"), QStringLiteral("UTF-8"), QStringLiteral("UTF-16"), QStringLiteral("UTF-16BE"), QStringLiteral("UTF-16LE"), QStringLiteral("UTF-32"), QStringLiteral("UTF-32BE"), QStringLiteral("UTF-32LE"), QStringLiteral("KOI8-R"), QStringLiteral("KOI8-U"), QStringLiteral("Big5-HKSCS"), QStringLiteral("GB18030"), QStringLiteral("EUC-JP"), QStringLiteral("EUC-KR"), QStringLiteral("ISO 2022-JP"), QStringLiteral("Shift-JIS"), QStringLiteral("Windows-949"), QStringLiteral("Windows-1251")}}
    };

    QTest::addColumn<File *>("bibliography");
    QTest::addColumn<QString>("encoding");

    for (const TestCase &testCase : testCases) {
        for (const QString &encoding : testCase.encodingsToBeTested) {
            const QString testLabel = QString(QStringLiteral("Round-trip of '%1' encoded in '%2'")).arg(testCase.label).arg(encoding);
            QTest::newRow(testLabel.toLatin1().constData()) << testCase.bibliography << encoding;
        }
    }
}

void KBibTeXIOTest::fileImportExportBibTeXroundtrip()
{
    QFETCH(File *, bibliography);
    QFETCH(QString, encoding);

    FileImporterBibTeX importer(this);

    // The point with this test is to apply various encodings (e.g. 'UTF-8', 'ISO 2022-JP')
    // to example bibliography files when writing to a buffer (a in-memory representation of
    // a real .bib file).
    // The encoding can by set in two different ways:
    // 1. As a property of the File object
    // 2. Enforced upon a FileExporterBibTeX instance, ignoring the File's encoding property

    // Fist, the forced-upon case will be executed

    QByteArray ba(1 << 12, '\0');
    QBuffer buffer(&ba);

    buffer.open(QBuffer::WriteOnly);
    FileExporterBibTeX exporterWithForcedEncoding(this);
    exporterWithForcedEncoding.setEncoding(encoding); //< Force the encoding on the FileExporterBibTeX instance
    QVERIFY(exporterWithForcedEncoding.save(&buffer, bibliography));
    const qint64 bytesWrittenWithForcedEncoding = buffer.pos();
    QVERIFY(bytesWrittenWithForcedEncoding > 32); //< All bibliographies in test have a certain minimum size
    buffer.close();

    ba.resize(static_cast<int>(bytesWrittenWithForcedEncoding & 0x7fffffff));

    buffer.open(QBuffer::ReadOnly);
    File *loadedFile = importer.load(&buffer);
    buffer.close();

    QVERIFY(loadedFile != nullptr);
    QVERIFY(loadedFile->length() == 1);
    QVERIFY(bibliography->operator ==(*loadedFile));
    delete loadedFile;

    // Second, the File encoding property case will be executed

    ba.fill('\0', 1 << 12); //< reset and clear buffer from above execution

    buffer.open(QBuffer::WriteOnly);
    FileExporterBibTeX exporterWithFileEncoding(this);
    bibliography->setProperty(File::Encoding, encoding); //< set the File's encoding property
    QVERIFY(exporterWithFileEncoding.save(&buffer, bibliography));
    const qint64 bytesWrittenWithFileEncoding = buffer.pos();
    QVERIFY(bytesWrittenWithFileEncoding > 32); //< All bibliographies in test have a certain minimum size
    buffer.close();

    ba.resize(static_cast<int>(bytesWrittenWithFileEncoding & 0x7fffffff));

    buffer.open(QBuffer::ReadOnly);
    loadedFile = importer.load(&buffer);
    buffer.close();

    QVERIFY(loadedFile != nullptr);
    QVERIFY(loadedFile->length() == 1);
    QVERIFY(bibliography->operator ==(*loadedFile));
    delete loadedFile;
}

void KBibTeXIOTest::protectiveCasingEntryFromData()
{
    static const QString titleText = QStringLiteral("Some Title for a Journal Article");
    static const QString singleCurleyBracketTitle = QStringLiteral("{") + titleText + QStringLiteral("}");
    static const QString doubleCurleyBracketTitle = QStringLiteral("{{") + titleText + QStringLiteral("}}");
    static const QString bibTeXDataDoubleCurleyBracketTitle = QStringLiteral("@articl{doubleCurleyBracketTitle,\ntitle={{") + titleText + QStringLiteral("}}\n}\n");
    static const QString bibTeXDataSingleCurleyBracketTitle = QStringLiteral("@articl{singleCurleyBracketTitle,\ntitle={") + titleText + QStringLiteral("}\n}\n");

    FileImporterBibTeX fileImporterBibTeX(this);
    FileExporterBibTeX fileExporterBibTeX(this);

    QByteArray b1(bibTeXDataDoubleCurleyBracketTitle.toUtf8());
    QBuffer bufferDoubleCurleyBracketTitle(&b1, this);
    QByteArray b2(bibTeXDataSingleCurleyBracketTitle.toUtf8());
    QBuffer bufferSingleCurleyBracketTitle(&b2, this);

    bufferDoubleCurleyBracketTitle.open(QBuffer::ReadOnly);
    QScopedPointer<File> fileDoubleCurleyBracketTitle(fileImporterBibTeX.load(&bufferDoubleCurleyBracketTitle));
    bufferDoubleCurleyBracketTitle.close();
    fileDoubleCurleyBracketTitle->setProperty(File::StringDelimiter, QStringLiteral("{}"));
    bufferSingleCurleyBracketTitle.open(QBuffer::ReadOnly);
    QScopedPointer<File> fileSingleCurleyBracketTitle(fileImporterBibTeX.load(&bufferSingleCurleyBracketTitle));
    bufferSingleCurleyBracketTitle.close();
    fileSingleCurleyBracketTitle->setProperty(File::StringDelimiter, QStringLiteral("{}"));

    fileDoubleCurleyBracketTitle->setProperty(File::ProtectCasing, Qt::PartiallyChecked);
    const QString textDoubleCurleyBracketTitlePartialProtectiveCasing = fileExporterBibTeX.toString(fileDoubleCurleyBracketTitle.data());
    QVERIFY(textDoubleCurleyBracketTitlePartialProtectiveCasing.contains(doubleCurleyBracketTitle));

    fileSingleCurleyBracketTitle->setProperty(File::ProtectCasing, Qt::PartiallyChecked);
    const QString textSingleCurleyBracketTitlePartialProtectiveCasing = fileExporterBibTeX.toString(fileSingleCurleyBracketTitle.data());
    QVERIFY(textSingleCurleyBracketTitlePartialProtectiveCasing.contains(singleCurleyBracketTitle)
            && !textSingleCurleyBracketTitlePartialProtectiveCasing.contains(doubleCurleyBracketTitle));

    fileDoubleCurleyBracketTitle->setProperty(File::ProtectCasing, Qt::Checked);
    const QString textDoubleCurleyBracketTitleWithProtectiveCasing = fileExporterBibTeX.toString(fileDoubleCurleyBracketTitle.data());
    QVERIFY(textDoubleCurleyBracketTitleWithProtectiveCasing.contains(doubleCurleyBracketTitle));

    fileSingleCurleyBracketTitle->setProperty(File::ProtectCasing, Qt::Checked);
    const QString textSingleCurleyBracketTitleWithProtectiveCasing = fileExporterBibTeX.toString(fileSingleCurleyBracketTitle.data());
    QVERIFY(textSingleCurleyBracketTitleWithProtectiveCasing.contains(doubleCurleyBracketTitle));

    fileDoubleCurleyBracketTitle->setProperty(File::ProtectCasing, Qt::Unchecked);
    const QString textDoubleCurleyBracketTitleWithoutProtectiveCasing = fileExporterBibTeX.toString(fileDoubleCurleyBracketTitle.data());
    QVERIFY(textDoubleCurleyBracketTitleWithoutProtectiveCasing.contains(singleCurleyBracketTitle)
            && !textDoubleCurleyBracketTitleWithoutProtectiveCasing.contains(doubleCurleyBracketTitle));

    fileSingleCurleyBracketTitle->setProperty(File::ProtectCasing, Qt::Unchecked);
    const QString textSingleCurleyBracketTitleWithoutProtectiveCasing = fileExporterBibTeX.toString(fileSingleCurleyBracketTitle.data());
    QVERIFY(textSingleCurleyBracketTitleWithoutProtectiveCasing.contains(singleCurleyBracketTitle)
            && !textSingleCurleyBracketTitleWithoutProtectiveCasing.contains(doubleCurleyBracketTitle));
}

void KBibTeXIOTest::partialBibTeXInput_data()
{
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<QString>("text");

    static const struct BibTeXDataTable {
        const char *label;
        const bool isValid;
        const QString text;
    }
    bibTeXDataTable[] = {
        {"Empty string", false, QString()},
        {"Only 'at' sign", false, QStringLiteral("@")},
        {"Only 'at' sign followed by element type", false, QStringLiteral("@entry")},
        {"Only up to opening curly bracket", false, QStringLiteral("@entry{")},
        {"Complete entry but without id", true, QStringLiteral("@entry{,\n  title=\"{Abc Def}\",\n  month = jan\n}")},
        {"Entry without any data", true, QStringLiteral("@entry{}")},
        {"Entry up to entry id, but no closing curly bracket", false, QStringLiteral("@entry{test")},
        {"Entry up to entry id with opening curly bracket", false, QStringLiteral("@entry{test{")},
        {"Entry up to entry id with closing curly bracket", true, QStringLiteral("@entry{test}")},
        {"Entry up to comma after entry id", false, QStringLiteral("@entry{test,")},
        {"Entry up to comma after entry id, followed by closing curly bracket", true, QStringLiteral("@entry{test,}")},
        {"Entry up to first field's key, but nothing more, not even an assign char", false, QStringLiteral("@entry{test,title")},
        {"Entry up to first field's key, but nothing more, just a closing curly bracket", false, QStringLiteral("@entry{test,title}")},
        {"Entry up to first field's assign char, but nothing more", false, QStringLiteral("@entry{test,title=")},
        {"Entry up to first field's assign char, but nothing more, just a closing curly bracket", false, QStringLiteral("@entry{test,title=}")},
        {"Invalid combination of curly bracket in a field's value (1)", false, QStringLiteral("@entry{test,title={}")},
        {"Invalid combination of curly bracket in a field's value (2)", false, QStringLiteral("@entry{test,title={{}}")},
        {"Invalid combination of curly bracket in a field's value (3)", false, QStringLiteral("@entry{test,title={}{}")},
        {"Invalid combination of curly bracket in a field's value (4)", false, QStringLiteral("@entry{test,title={}{}}")},
        {"Complete entry with empty title (1)", true, QStringLiteral("@entry{test,\n  title=\"{}\"\n}")},
        {"Complete entry with empty title (2)", true, QStringLiteral("@entry{test,\n  title=\"\"\n}")},
        {"Complete entry with empty title (3)", true, QStringLiteral("@entry{test,\n  title={{}}\n}")},
        {"Complete entry with empty title (4)", true, QStringLiteral("@entry{test,\n  title={}\n}")},
        {"Entry abruptly ending at macro key as field value (1)", false, QStringLiteral("@entry{test,\n  month = jan")},
        {"Entry abruptly ending at macro key as field value (2)", false, QStringLiteral("@entry{test,\n  month = jan\n")},
        // TODO more tests
        {"Complete entry", true, QStringLiteral("@entry{test,\n  title=\"{Abc Def}\",\n  month = jan\n}")}
    };

    for (const auto &bibTeXDataRow : bibTeXDataTable)
        QTest::newRow(bibTeXDataRow.label) << bibTeXDataRow.isValid << bibTeXDataRow.text;
}

void KBibTeXIOTest::partialBibTeXInput()
{
    QFETCH(bool, isValid);
    QFETCH(QString, text);

    bool gotErrors = false;
    FileImporterBibTeX importer(this);
    connect(&importer, &FileImporter::message, [&gotErrors](const FileImporter::MessageSeverity messageSeverity, const QString &messageText) {
        gotErrors |= messageSeverity >= FileImporter::MessageSeverity::Error;
        Q_UNUSED(messageText)
        //qCDebug(LOG_KBIBTEX_TEST)<<"FileImporterBibTeX issues message during 'partialBibTeXInput' test: "<<messageText;
    });
    QScopedPointer<File> bibTeXfile(importer.fromString(text));

    QVERIFY(text.isEmpty() || isValid != gotErrors);
    QVERIFY(isValid ? (!bibTeXfile.isNull() && bibTeXfile->count() == 1) : (bibTeXfile.isNull() || bibTeXfile->count() == 0));
}

void KBibTeXIOTest::partialRISInput_data()
{
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<QString>("text");

    static const struct RISDataTable {
        const char *label;
        const bool isValid;
        const QString text;
    }
    risDataTable[] = {
        //{"Empty string", false, QString()},
        {"Incorrect year", true, QStringLiteral("TY  - JOUR\nAU  - Shannon, Claude E.\nPY  - 5555/07//\nTI  - A Mathematical Theory of Communication\nT2  - Bell System Technical Journal\nSP  - 379\nEP  - 423\nVL  - 27\nER  -")},
        {"Incorrect month", true, QStringLiteral("TY  - JOUR\nAU  - Shannon, Claude E.\nPY  - 1948/17//\nTI  - A Mathematical Theory of Communication\nT2  - Bell System Technical Journal\nSP  - 379\nEP  - 423\nVL  - 27\nER  -")},
        {"Entry does not end with 'ER'", true, QStringLiteral("TY  - JOUR\nAU  - Shannon, Claude E.\nPY  - 1948/07//\nTI  - A Mathematical Theory of Communication\nT2  - Bell System Technical Journal\nSP  - 379\nEP  - 423\nVL  - 27")},
        // TODO more tests
        //{"Complete entry", true, QStringLiteral("TY  - JOUR\nAU  - Shannon, Claude E.\nPY  - 1948/07//\nTI  - A Mathematical Theory of Communication\nT2  - Bell System Technical Journal\nSP  - 379\nEP  - 423\nVL  - 27\nER  -")}
    };

    for (const auto &risDataRow : risDataTable)
        QTest::newRow(risDataRow.label) << risDataRow.isValid << risDataRow.text;
}

void KBibTeXIOTest::partialRISInput()
{
    QFETCH(bool, isValid);
    QFETCH(QString, text);

    bool gotErrors = false;
    FileImporterRIS importer(this);
    connect(&importer, &FileImporter::message, [&gotErrors](const FileImporter::MessageSeverity messageSeverity, const QString &messageText) {
        gotErrors |= messageSeverity >= FileImporter::MessageSeverity::Error;
        Q_UNUSED(messageText)
        //qCDebug(LOG_KBIBTEX_TEST)<<"FileImporterRIS issues message during 'partialBibTeXInput' test: "<<messageText;
    });
    QScopedPointer<File> bibTeXfile(importer.fromString(text));

    QVERIFY(text.isEmpty() || isValid != gotErrors);
    QVERIFY(isValid ? (!bibTeXfile.isNull() && bibTeXfile->count() == 1) : (bibTeXfile.isNull() || bibTeXfile->count() == 0));
}

void KBibTeXIOTest::initTestCase()
{
    qRegisterMetaType<FileImporter::MessageSeverity>();
}

QTEST_MAIN(KBibTeXIOTest)

#include "kbibtexiotest.moc"
