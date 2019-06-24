/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

Q_DECLARE_METATYPE(QMimeType)

class KBibTeXIOTest : public QObject
{
    Q_OBJECT

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
    void fileImporterSplitName_data();
    void fileImporterSplitName();
    void fileInfoMimeTypeForUrl_data();
    void fileInfoMimeTypeForUrl();
    void fileInfoUrlsInText_data();
    void fileInfoUrlsInText();
    QVector<QPair<const char *, File *> > fileImporterExporterTestCases();
    void fileExporterXMLsave_data();
    void fileExporterXMLsave();
    void fileExporterRISsave_data();
    void fileExporterRISsave();
    void fileExporterBibTeXsave_data();
    void fileExporterBibTeXsave();
    void fileImporterRISload_data();
    void fileImporterRISload();
    void fileImporterBibTeXload_data();
    void fileImporterBibTeXload();
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
    QTest::addColumn<QString>("asciialternative1");
    QTest::addColumn<QString>("asciialternative2");

    QTest::newRow("Just ASCII letters and numbers") << QStringLiteral("qwertyuiopASDFGHJKLzxcvbnm1234567890") << QStringLiteral("qwertyuiopASDFGHJKLzxcvbnm1234567890") << QString();
    QTest::newRow("ASCII low and high bytes") << QStringLiteral("\x00\x01\x09\x0a\x10\x11\x19\x1a\x1f\x20\x7e\x7f") << QStringLiteral(" ~") << QString();
    QTest::newRow("European Scripts/Latin-1 Supplement") << QString::fromUtf8("\xc3\x80\xc3\x82\xc3\x84\xc3\x92\xc3\x94\xc3\x96\xc3\xac\xc3\xad\xc3\xae\xc3\xaf") << QStringLiteral("AAAOOOiiii") << QStringLiteral("AAAEOOOEiiii");
    QTest::newRow("European Scripts/Latin Extended-A") << QString::fromUtf8("\xc4\x8a\xc4\x8b\xc4\xae\xc4\xaf\xc5\x9c\xc5\x9d\xc5\xbb\xc5\xbc") << QStringLiteral("CcIiSsZz") << QString();
    QTest::newRow("European Scripts/Latin Extended-B") << QString::fromUtf8("\xc7\x8a\xc7\x8b\xc7\x8c") << QStringLiteral("NJNjnj") << QString();
    QTest::newRow("European Scripts/Latin Extended Additional") << QString::fromUtf8("\xe1\xb8\xbe\xe1\xb8\xbf\xe1\xb9\xa4\xe1\xb9\xa5\xe1\xbb\xae\xe1\xbb\xaf") << QStringLiteral("MmSsUu") << QString();
    QTest::newRow("European Scripts/Cyrillic") << QString::fromUtf8("\xd0\x90\xd0\x9e\xd0\x9f") << QStringLiteral("AOP") << QString();
    QTest::newRow("European Scripts/Greek and Coptic") << QString::fromUtf8("\xce\xba\xce\xb1\xce\xa4\xcf\xba\xce\x9d") << QStringLiteral("kaTSN") << QStringLiteral("kappaalphaTauSanNu");
    QTest::newRow("East Asian Scripts/Katakana") << QString::fromUtf8("\xe3\x82\xb7\xe3\x83\x84") << QStringLiteral("shitsu") << QStringLiteral("situ");
    QTest::newRow("East Asian Scripts/Hangul Syllables") << QString::fromUtf8("\xea\xb9\x80\xec\xa0\x95\xec\x9d\x80") << QStringLiteral("gimjeongeun") << QStringLiteral("gimjeong-eun");
}

void KBibTeXIOTest::encoderConvertToPlainAscii()
{
    QFETCH(QString, unicodestring);
    QFETCH(QString, asciialternative1);
    QFETCH(QString, asciialternative2);

    const QString converted = Encoder::instance().convertToPlainAscii(unicodestring);
    if (converted != asciialternative1 && converted != asciialternative2)
        qWarning() << "converted=" << converted << "  asciialternative1=" << asciialternative1 << "  asciialternative2=" << asciialternative2;
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

    QCOMPARE(EncoderXML::instance().encode(unicode, Encoder::TargetEncodingASCII), xml);
}

void KBibTeXIOTest::encoderLaTeXdecode_data()
{
    QTest::addColumn<QString>("latex");
    QTest::addColumn<QString>("unicode");
    QTest::addColumn<QString>("alternativelatex");

    QTest::newRow("Just ASCII") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QString();
    QTest::newRow("Dotless i and j characters") << QStringLiteral("{\\`\\i}{\\'\\i}{\\^\\i}{\\\"\\i}{\\~\\i}{\\=\\i}{\\u\\i}{\\k\\i}{\\^\\j}{\\v\\i}{\\v\\j}") << QString(QChar(0x00EC)) + QChar(0x00ED) + QChar(0x00EE) + QChar(0x00EF) + QChar(0x0129) + QChar(0x012B) + QChar(0x012D) + QChar(0x012F) + QChar(0x0135) + QChar(0x01D0) + QChar(0x01F0) << QString();
    QTest::newRow("\\l and \\ldots") << QStringLiteral("\\l\\ldots\\l\\ldots") << QString(QChar(0x0142)) + QChar(0x2026) + QChar(0x0142) + QChar(0x2026) << QStringLiteral("{\\l}{\\ldots}{\\l}{\\ldots}");
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

    const QString generatedLatex = EncoderLaTeX::instance().encode(unicode, Encoder::TargetEncodingASCII);
    if (generatedLatex != latex && !alternativelatex.isEmpty())
        QCOMPARE(generatedLatex, alternativelatex);
    else
        QCOMPARE(generatedLatex, latex);
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
    QTest::newRow("Lore ipsum with DOI (with URL)") << QStringLiteral("Lore ipsum http://doi.example.org/10.1000/38-abc Lore ipsum") << QSet<QUrl> {QUrl(KBibTeX::doiUrlPrefix + QStringLiteral("10.1000/38-abc"))};
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
    FileInfo::urlsInText(text, FileInfo::TestExistenceNo, QString(), extractedUrls);

    QCOMPARE(extractedUrls.count(), expectedUrls.count());
    for (const QUrl &expectedUrl : const_cast<const QSet<QUrl> &>(expectedUrls))
        QCOMPARE(extractedUrls.contains(expectedUrl), true);
}

QVector<QPair<const char *, File *> > KBibTeXIOTest::fileImporterExporterTestCases()
{
    static QVector<QPair<const char *, File *> > result;

    if (result.isEmpty()) {
        /// Empty file without any entries
        result.append(QPair<const char *, File *>("Empty file", new File()));

        /// File with single entry, inspired by 'Moby Dick'
        File *f1 = new File();
        QSharedPointer<Entry> entry1(new Entry(Entry::etArticle, QStringLiteral("the-whale-1851")));
        f1->append(entry1);
        entry1->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("{Call me Ishmael}"))));
        entry1->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Herman"), QStringLiteral("Melville"))) << QSharedPointer<Person>(new Person(QStringLiteral("Moby"), QStringLiteral("Dick"))));
        entry1->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("1851"))));
        result.append(QPair<const char *, File *>("Moby Dick", f1));

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
        {"Empty file", QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>|<!-- XML document written by KBibTeXIO as part of KBibTeX -->|<!-- https://userbase.kde.org/KBibTeX -->|<bibliography>|</bibliography>|")},
        {"Moby Dick", QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>|<!-- XML document written by KBibTeXIO as part of KBibTeX -->|<!-- https://userbase.kde.org/KBibTeX -->|<bibliography>| <entry id=\"the-whale-1851\" type=\"article\">|  <authors>|<person><firstname>Herman</firstname><lastname>Melville</lastname></person> <person><firstname>Moby</firstname><lastname>Dick</lastname></person>|  </authors>|  <title><text>Call me Ishmael</text></title>|  <year><text>1851</text></year>| </entry>|</bibliography>|")}
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
    QStringList errorLog;
    const QString generatedData = fileExporterXML.toString(bibTeXfile, &errorLog).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|'));
    for (const QString &logLine : const_cast<const QStringList &>(errorLog))
        qDebug() << logLine;

    QCOMPARE(generatedData, xmlData);
}

void KBibTeXIOTest::fileExporterRISsave_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("risData");

    static const QHash<const char *, QString> keyToRisData {
        {"Empty file", QString()},
        {"Moby Dick", QStringLiteral("TY  - JOUR|ID  - the-whale-1851|AU  - Melville, Herman|AU  - Dick, Moby|TI  - Call me Ishmael|PY  - 1851///|ER  - ||")}
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
    QStringList errorLog;
    const QString generatedData = fileExporterRIS.toString(bibTeXfile, &errorLog).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|'));
    for (const QString &logLine : const_cast<const QStringList &>(errorLog))
        qDebug() << logLine;

    QCOMPARE(generatedData, risData);
}

void KBibTeXIOTest::fileExporterBibTeXsave_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("bibTeXdata");

    static const QHash<const char *, QString> keyToBibTeXData {
        {"Empty file", QString()},
        {"Moby Dick", QStringLiteral("@article{the-whale-1851,|\tauthor = {Melville, Herman and Dick, Moby},|\ttitle = {{Call me Ishmael}},|\tyear = {1851}|}||")}
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
    QStringList errorLog;
    const QString generatedData = fileExporterBibTeX.toString(bibTeXfile, &errorLog).remove(QLatin1Char('\r')).replace(QLatin1Char('\n'), QLatin1Char('|'));
    for (const QString &logLine : const_cast<const QStringList &>(errorLog))
        qDebug() << logLine;

    QCOMPARE(generatedData, bibTeXdata);
}

void KBibTeXIOTest::fileImporterRISload_data()
{
    QTest::addColumn<QByteArray>("risData");
    QTest::addColumn<File *>("bibTeXfile");

    static const QHash<const char *, QString> keyToRisData {
        {"Empty file", QString()},
        {"Moby Dick", QStringLiteral("TY  - JOUR|ID  - the-whale-1851|AU  - Melville, Herman|AU  - Dick, Moby|TI  - Call me Ishmael|PY  - 1851///|ER  - ||")}
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
        {"Empty file", QString()},
        {"Moby Dick", QStringLiteral("@article{the-whale-1851,|\tauthor = {Melville, Herman and Dick, Moby},|\ttitle = {{Call me Ishmael}},|\tyear = {1851}|}||")}
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
        gotErrors |= messageSeverity >= FileImporter::SeverityError;
        Q_UNUSED(messageText);
        //qDebug()<<"FileImporterBibTeX issues message during 'partialBibTeXInput' test: "<<messageText;
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
        gotErrors |= messageSeverity >= FileImporter::SeverityError;
        Q_UNUSED(messageText);
        //qDebug()<<"FileImporterRIS issues message during 'partialBibTeXInput' test: "<<messageText;
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
