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

#include <QtTest>

#include <QStandardPaths>

#include "encoderxml.h"
#include "encoderlatex.h"
#include "value.h"
#include "entry.h"
#include "fileimporter.h"
#include "fileexporterris.h"
#include "fileexporterbibtex.h"
#include "fileexporterxml.h"
#include "fileimporterbibtex.h"
#include "fileimporterris.h"
#include "fileinfo.h"
#include "preferences.h"

Q_DECLARE_METATYPE(QMimeType)

class KBibTeXIOTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
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

private:
};

void KBibTeXIOTest::encoderXMLdecode_data()
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<QString>("unicode");

    QTest::newRow("Just ASCII") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.") << QStringLiteral("Gallia est omnis divisa in partes tres, quarum unam incolunt Belgae, aliam Aquitani, tertiam qui ipsorum lingua Celtae, nostra Galli appellantur.");
    QTest::newRow("Quotation marks") << QStringLiteral("Caesar said: &quot;Veni, vidi, vici&quot;") << QStringLiteral("Caesar said: \"Veni, vidi, vici\"");
    QTest::newRow("Characters from EncoderXMLCharMapping") << QStringLiteral("&quot;&amp;&lt;&gt;") << QStringLiteral("\"\\&<>");
    QTest::newRow("Characters from backslashSymbols") << QStringLiteral("&amp;%_") << QStringLiteral("\\&\\%\\_");

    for (int start = 0; start < 10; ++start) {
        QString xmlString, unicodeString;
        for (int offset = 1561; offset < 6791; offset += 621) {
            const ushort unicode = start * 3671 + offset;
            xmlString += QStringLiteral("&#") + QString::number(unicode) + QStringLiteral(";");
            unicodeString += QChar(unicode);
        }
        QTest::newRow(QString(QStringLiteral("Some arbitrary Unicode characters (%1)")).arg(start).toLatin1().constData()) << xmlString << unicodeString;
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
    QTest::newRow("Dotless i and j characters") << QStringLiteral("\\`{\\i}\\'{\\i}\\^{\\i}\\\"{\\i}\\~{\\i}\\={\\i}\\u{\\i}\\k{\\i}\\^{\\j}\\v{\\i}\\v{\\j}") << QString(QChar(0x00EC)) + QChar(0x00ED) + QChar(0x00EE) + QChar(0x00EF) + QChar(0x0129) + QChar(0x012B) + QChar(0x012D) + QChar(0x012F) + QChar(0x0135) + QChar(0x01D0) + QChar(0x01F0) << QString();
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
    QTest::newRow("Lore ipsum with DOI (without URL)") << QStringLiteral("Lore ipsum 10.1000/38-abc Lore ipsum") << QSet<QUrl>{QUrl(FileInfo::doiUrlPrefix() + QStringLiteral("10.1000/38-abc"))};
    QTest::newRow("Lore ipsum with DOI (with URL)") << QStringLiteral("Lore ipsum http://doi.example.org/10.1000/38-abc Lore ipsum") << QSet<QUrl>{QUrl(FileInfo::doiUrlPrefix() + QStringLiteral("10.1000/38-abc"))};
    QTest::newRow("URLs and DOI (without URL), all semicolon-separated") << QStringLiteral("http://www.example.com;10.1000/38-abc   ;\nhttps://www.example.com") << QSet<QUrl>{QUrl(QStringLiteral("http://www.example.com")), QUrl(FileInfo::doiUrlPrefix() + QStringLiteral("10.1000/38-abc")), QUrl(QStringLiteral("https://www.example.com"))};
    QTest::newRow("URLs and DOI (with URL), all semicolon-separated") << QStringLiteral("http://www.example.com\n;   10.1000/38-abc;https://www.example.com") << QSet<QUrl>{QUrl(QStringLiteral("http://www.example.com")), QUrl(FileInfo::doiUrlPrefix() + QStringLiteral("10.1000/38-abc")), QUrl(QStringLiteral("https://www.example.com"))};
    QTest::newRow("URLs with various separators") << QStringLiteral("http://www.example.com/def.pdf https://www.example.com\nhttp://download.example.com/abc") << QSet<QUrl>{QUrl(QStringLiteral("http://www.example.com/def.pdf")), QUrl(QStringLiteral("https://www.example.com")), QUrl(QStringLiteral("http://download.example.com/abc"))};
    QTest::newRow("URLs with query strings and anchors") << QStringLiteral("http://www.example.com/def.pdf?a=3&b=1 https://www.example.com#1581584\nhttp://download.example.com/abc,7352,A#abc?gh=352&ghi=1254") << QSet<QUrl>{QUrl(QStringLiteral("http://www.example.com/def.pdf?a=3&b=1")), QUrl(QStringLiteral("https://www.example.com#1581584")), QUrl(QStringLiteral("http://download.example.com/abc,7352,A#abc?gh=352&ghi=1254"))};
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

    static const QMap<const char *, QString> keyToXmlData {
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
    for (const QString &logLine : errorLog)
        qDebug() << logLine;

    QCOMPARE(generatedData, xmlData);
}

void KBibTeXIOTest::fileExporterRISsave_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("risData");

    static const QMap<const char *, QString> keyToRisData {
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
    for (const QString &logLine : errorLog)
        qDebug() << logLine;

    QCOMPARE(generatedData, risData);
}

void KBibTeXIOTest::fileExporterBibTeXsave_data()
{
    QTest::addColumn<File *>("bibTeXfile");
    QTest::addColumn<QString>("bibTeXdata");

    static const QMap<const char *, QString> keyToBibTeXData {
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
    for (const QString &logLine : errorLog)
        qDebug() << logLine;

    QCOMPARE(generatedData, bibTeXdata);
}

void KBibTeXIOTest::fileImporterRISload_data()
{
    QTest::addColumn<QByteArray>("risData");
    QTest::addColumn<File *>("bibTeXfile");

    static const QMap<const char *, QString> keyToRisData {
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
    File *generatedFile = fileImporterRIS.load(&buffer);

    QVERIFY(generatedFile->operator ==(*bibTeXfile));

    delete generatedFile;
}

void KBibTeXIOTest::fileImporterBibTeXload_data()
{
    QTest::addColumn<QByteArray>("bibTeXdata");
    QTest::addColumn<File *>("bibTeXfile");

    static const QMap<const char *, QString> keyToBibTeXData {
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
    File *generatedFile = fileImporterBibTeX.load(&buffer);

    QVERIFY(generatedFile->operator ==(*bibTeXfile));

    delete generatedFile;
}

void KBibTeXIOTest::initTestCase()
{
    QFile texFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QStringLiteral("/encoderlatex-tables.tex"));
    qDebug() << "Writing LaTeX tables to: " << texFile.fileName();
    if (texFile.open(QFile::WriteOnly)) {
        EncoderLaTeX::writeLaTeXTables(texFile);
        texFile.close();
    }
}

QTEST_MAIN(KBibTeXIOTest)

#include "kbibtexiotest.moc"
