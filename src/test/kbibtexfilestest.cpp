/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *   Copyright (C) 2014 by Pino Toscano <pino@kde.org>                     *
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

#include <QCryptographicHash>
#include <QTemporaryFile>

#include <QDebug>
#ifdef WRITE_RAWDATAFILE
#include <QFile>
#endif // WRITE_RAWDATAFILE

#include "entry.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"
/// Provides definition of TESTSET_DIRECTORY
#include "test-config.h"
#include "kbibtexfilestest-rawdata.h"

typedef struct {
    QString filename;
    int numElements, numEntries;
    QString lastEntryId, lastEntryLastAuthorLastName;
    QByteArray hashAuthors, hashFilesUrlsDoi;
} TestFile;

Q_DECLARE_METATYPE(TestFile)

class KBibTeXFilesTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
#ifdef WRITE_RAWDATAFILE
    void cleanupTestCase();
#endif // WRITE_RAWDATAFILE
    void testFiles_data();
    void testFiles();

private:
    /**
     * Load a bibliography file and checks a number of known properties
     * such as number of elements/entries or the hash sum of authors' last names.
     * It is the caller's responsibility to pass a valid argument to @p outFile
     * and later delete the returned File object.
     *
     * @param absoluteFilename biblography file to laod
     * @param currentTestFile data structure holding the baseline values
     * @param outFile returns pointer to the opened file
     */
    void loadFile(const QString &absoluteFilename, const TestFile &currentTestFile, File **outFile);

    /**
     * Save a bibliography in a temporary file.
     * It is the caller's responsibility to pass a valid argument to @p outFile,
     * which will hold the temporary file's name upon successful return.
     *
     * @param file bibliography data structure to be saved
     * @param currentTestFile baseline data structure used to determine temporary file's name
     * @param outFile returns the temporary file's name
     */
    void saveFile(File *file, const TestFile &currentTestFile, QString *outFile);

    /**
     * Create and fill a TestFile data structure based on the provided values.
     *
     * @param filename Bibliography file's filename
     * @param numElements Number of elements to expect in bibliography
     * @param numEntries Number of entries to expect in bibliography
     * @param lastEntryId Identifier of last entry in bibliography
     * @param lastEntryLastAuthorLastName Last author's last name in bibliography
     * @param hashAuthors The hash sum over all authors/editors in bibliography
     * @param hashFilesUrlsDoi The hash sum over all URLs and DOIs in bibliography
     * @return An initialized TestFile data structure
     */
    TestFile createTestFile(const QString &filename, int numElements, int numEntries, const QString &lastEntryId, const QString &lastEntryLastAuthorLastName, const QByteArray &hashAuthors, const QByteArray &hashFilesUrlsDoi);

#ifdef WRITE_RAWDATAFILE
    static QString rewriteNonASCII(const QString &input);
#endif // WRITE_RAWDATAFILE
};


void KBibTeXFilesTest::initTestCase()
{
    qRegisterMetaType<TestFile>("TestFile");

#ifdef WRITE_RAWDATAFILE
    QFile rawDataFile("kbibtexfilestest-rawdata.h");
    if (rawDataFile.open(QFile::WriteOnly)) {
        QTextStream ts(&rawDataFile);
        ts << QStringLiteral("/********************************************************************************") << endl << endl;
        ts << QStringLiteral("Copyright ") << QDate::currentDate().year() << QStringLiteral("  Thomas Fischer <fischer@unix-ag.uni-kl.de> and others") << endl << endl;
        ts << QStringLiteral("Redistribution and use in source and binary forms, with or without") << endl << QStringLiteral("modification, are permitted provided that the following conditions") << endl << QStringLiteral("are met:") << endl << endl;
        ts << QStringLiteral("1. Redistributions of source code must retain the above copyright") << endl << QStringLiteral("   notice, this list of conditions and the following disclaimer.") << endl;
        ts << QStringLiteral("2. Redistributions in binary form must reproduce the above copyright") << endl << QStringLiteral("   notice, this list of conditions and the following disclaimer in the") << endl << QStringLiteral("   documentation and/or other materials provided with the distribution.") << endl << endl;
        ts << QStringLiteral("THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR") << endl << QStringLiteral("IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES") << endl << QStringLiteral("OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.") << endl << QStringLiteral("IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,") << endl << QStringLiteral("INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT") << endl << QStringLiteral("NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,") << endl << QStringLiteral("DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY") << endl << QStringLiteral("THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT") << endl << QStringLiteral("(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF") << endl << QStringLiteral("THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.") << endl << endl;
        ts << QStringLiteral("********************************************************************************/") << endl << endl;
        ts << QStringLiteral("#ifndef KBIBTEX_FILES_TEST_RAWDATA_H") << endl << QStringLiteral("#define KBIBTEX_FILES_TEST_RAWDATA_H") << endl << endl;
        rawDataFile.close();
    }
#endif // WRITE_RAWDATAFILE
}

#ifdef WRITE_RAWDATAFILE
void KBibTeXFilesTest::cleanupTestCase()
{
    QFile rawDataFile("kbibtexfilestest-rawdata.h");
    if (rawDataFile.open(QFile::Append)) {
        QTextStream ts(&rawDataFile);
        ts << endl << QStringLiteral("#endif // KBIBTEX_FILES_TEST_RAWDATA_H") << endl;
        rawDataFile.close();
    }
}
#endif // WRITE_RAWDATAFILE

void KBibTeXFilesTest::testFiles_data()
{
    QTest::addColumn<TestFile>("testFile");

    QTest::newRow("bug19489.bib") << createTestFile(QStringLiteral("bib/bug19489.bib"), 1, 1, QStringLiteral("bart:04:1242"), QStringLiteral("Ralph"), QCryptographicHash::hash(bug19489LastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(bug19489FilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("names-with-braces.bib") << createTestFile(QStringLiteral("bib/names-with-braces.bib"), 1, 1, QStringLiteral("names1"), QStringLiteral("{{{{{LastName3A LastName3B}}}}}"), QCryptographicHash::hash(nameswithbracesLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(nameswithbracesFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("duplicates.bib") << createTestFile(QStringLiteral("bib/duplicates.bib"), 23, 23, QStringLiteral("books/aw/Sedgewick88"), QStringLiteral("Sedgewick"), QCryptographicHash::hash(duplicatesLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(duplicatesFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("minix.bib") << createTestFile(QStringLiteral("bib/minix.bib"), 163, 123, QStringLiteral("Jesshope:2006:ACS"), QStringLiteral("Egan"), QCryptographicHash::hash(minixLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(minixFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("bug19484-refs.bib") << createTestFile(QStringLiteral("bib/bug19484-refs.bib"), 641, 641, QStringLiteral("Bagnara-etal-2002"), QStringLiteral("Hill"), QCryptographicHash::hash(bug19484refsLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(bug19484refsFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("bug19362-file15701-database.bib") << createTestFile(QStringLiteral("bib/bug19362-file15701-database.bib"), 911, 911, QStringLiteral("New1"), QStringLiteral("Sunder"), QCryptographicHash::hash(bug19362file15701databaseLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(bug19362file15701databaseFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("digiplay.bib") << createTestFile(QStringLiteral("bib/digiplay.bib"), 3074, 3074, QStringLiteral("1180"), QStringLiteral("Huizinga"), QCryptographicHash::hash(digiplayLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(digiplayFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("backslash.bib") << createTestFile(QStringLiteral("bib/backslash.bib"), 1, 1, QStringLiteral("backslash-test"), QStringLiteral("Doe"), QCryptographicHash::hash(backslashLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(backslashFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("bug379443-attachment105313-IOPEXPORT_BIB.bib") << createTestFile(QStringLiteral("bib/bug379443-attachment105313-IOPEXPORT_BIB.bib"), 1, 1, QStringLiteral("1748-0221-3-08-S08004"), QStringLiteral("Yuldashev"), QCryptographicHash::hash(bug379443attachment105313IOPEXPORTBIBLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(bug379443attachment105313IOPEXPORTBIBFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("bug21870-polito.bib") << createTestFile(QStringLiteral("bib/bug21870-polito.bib"), 736, 721, QStringLiteral("BusseNiehrWengeler:Brisante05"), QStringLiteral("Wengeler"), QCryptographicHash::hash(bug21870politoLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(bug21870politoFilesUrlsDois, QCryptographicHash::Md5));
    QTest::newRow("cloud-duplicates.bib") << createTestFile(QStringLiteral("bib/cloud-duplicates.bib"), 21, 21, QStringLiteral("rao2012survey"), QStringLiteral("Reddy"), QCryptographicHash::hash(cloudduplicatesLastAuthors, QCryptographicHash::Md4), QCryptographicHash::hash(cloudduplicatesFilesUrlsDois, QCryptographicHash::Md5));
}

void KBibTeXFilesTest::testFiles()
{
    QFETCH(TestFile, testFile);

    const QString absoluteFilename = QLatin1String(TESTSET_DIRECTORY "/") + testFile.filename;
    QVERIFY(QFileInfo::exists(absoluteFilename));

    /// First load the file ...
    File *file = nullptr;
    loadFile(absoluteFilename, testFile, &file);
    QVERIFY(file);

#ifndef WRITE_RAWDATAFILE
    /// ... then save it again to file ...
    QString tempFileName;
    saveFile(file, testFile, &tempFileName);
    QVERIFY(!tempFileName.isEmpty());

    /// ... and finally try to load again the newly saved version
    File *file2 = nullptr;
    loadFile(tempFileName, testFile, &file2);
    QVERIFY(file2);

    QFile::remove(tempFileName);
#endif // WRITE_RAWDATAFILE

    delete file;
#ifndef WRITE_RAWDATAFILE
    delete file2;
#endif // WRITE_RAWDATAFILE
}

void KBibTeXFilesTest::loadFile(const QString &absoluteFilename, const TestFile &currentTestFile, File **outFile)
{
    *outFile = nullptr;

    FileImporterBibTeX *importer = nullptr;
    if (currentTestFile.filename.endsWith(QStringLiteral(".bib"))) {
        importer = new FileImporterBibTeX(this);
        importer->setCommentHandling(FileImporterBibTeX::KeepComments);
    } else {
        QFAIL(qPrintable(QString::fromLatin1("Don't know format of '%1'").arg(currentTestFile.filename)));
    }

    QFile file(absoluteFilename);
    File *bibTeXFile = nullptr;
    QVERIFY(file.open(QFile::ReadOnly));
    bibTeXFile = importer->load(&file);
    file.close();

    QVERIFY(bibTeXFile);
    QVERIFY(!bibTeXFile->isEmpty());

    QStringList lastAuthorsList, filesUrlsDoiList;
    lastAuthorsList.reserve(bibTeXFile->size());
    int countElements = bibTeXFile->count(), countEntries = 0;
    QString lastEntryId, lastEntryLastAuthorLastName;
    for (const auto &element : const_cast<const File &>(*bibTeXFile)) {
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        if (!entry.isNull()) {
            ++countEntries;
            lastEntryId = entry->id();

            Value authors = entry->value(Entry::ftAuthor);
            if (!authors.isEmpty()) {
                ValueItem *vi = authors.last().data();
                Person *p = dynamic_cast<Person *>(vi);
                if (p != nullptr) {
                    lastEntryLastAuthorLastName = p->lastName();
                } else
                    lastEntryLastAuthorLastName.clear();
            } else {
                Value editors = entry->value(Entry::ftEditor);
                if (!editors.isEmpty()) {
                    ValueItem *vi = editors.last().data();
                    Person *p = dynamic_cast<Person *>(vi);
                    if (p != nullptr) {
                        lastEntryLastAuthorLastName = p->lastName();
                    } else
                        lastEntryLastAuthorLastName.clear();
                } else
                    lastEntryLastAuthorLastName.clear();
            }

            if (!lastEntryLastAuthorLastName.isEmpty()) {
                if (lastEntryLastAuthorLastName[0] == QLatin1Char('{') && lastEntryLastAuthorLastName[lastEntryLastAuthorLastName.length() - 1] == QLatin1Char('}'))
                    lastEntryLastAuthorLastName = lastEntryLastAuthorLastName.mid(1, lastEntryLastAuthorLastName.length() - 2);
                lastAuthorsList << lastEntryLastAuthorLastName;
            }

            for (int index = 1; index < 100; ++index) {
                const QString field = index == 1 ? Entry::ftUrl : QString(QStringLiteral("%1%2")).arg(Entry::ftUrl).arg(index);
                const Value v = entry->value(field);
                for (const QSharedPointer<ValueItem> &vi : v) {
                    filesUrlsDoiList << PlainTextValue::text(vi);
                }
                if (v.isEmpty() && index > 10) break;
            }
            for (int index = 1; index < 100; ++index) {
                const QString field = index == 1 ? Entry::ftDOI : QString(QStringLiteral("%1%2")).arg(Entry::ftDOI).arg(index);
                const Value v = entry->value(field);
                for (const QSharedPointer<ValueItem> &vi : v) {
                    filesUrlsDoiList << PlainTextValue::text(vi);
                }
                if (v.isEmpty() && index > 10) break;
            }
            for (int index = 1; index < 100; ++index) {
                const QString field = index == 1 ? Entry::ftLocalFile : QString(QStringLiteral("%1%2")).arg(Entry::ftLocalFile).arg(index);
                const Value v = entry->value(field);
                for (const QSharedPointer<ValueItem> &vi : v) {
                    filesUrlsDoiList << PlainTextValue::text(vi);
                }
                if (v.isEmpty() && index > 10) break;
            }
        }
    }

    QCOMPARE(countElements, currentTestFile.numElements);
    QCOMPARE(countEntries, currentTestFile.numEntries);
    QCOMPARE(lastEntryId, currentTestFile.lastEntryId);
    QCOMPARE(lastEntryLastAuthorLastName, currentTestFile.lastEntryLastAuthorLastName);

#ifdef WRITE_RAWDATAFILE
    static const QRegularExpression filenameStemRegExp(QStringLiteral("/?([^/]+)[.]bib$"));
    const QString filenameStem = filenameStemRegExp.match(currentTestFile.filename).captured(1).remove(QChar('-')).remove(QChar('_'));
    QFile rawDataFile("kbibtexfilestest-rawdata.h");
    static const size_t max_len = 256;
    size_t len;
#endif // WRITE_RAWDATAFILE

    QCryptographicHash hashAuthors(QCryptographicHash::Md4);
    lastAuthorsList.sort();
#ifdef WRITE_RAWDATAFILE
    QString sourceCode = QStringLiteral("static const char *") + filenameStem + QStringLiteral("LastAuthors(\"");
    len = 0;
#endif // WRITE_RAWDATAFILE
    for (const QString &lastAuthor : const_cast<const QStringList &>(lastAuthorsList)) {
        const QByteArray lastAuthorUtf8 = lastAuthor.toUtf8();
#ifdef WRITE_RAWDATAFILE
        sourceCode += rewriteNonASCII(QString(lastAuthor));
        len += lastAuthorUtf8.length();
        if (len > max_len) {
            sourceCode += QStringLiteral("\"\n        \"");
            len = 0;
        }
#endif // WRITE_RAWDATAFILE
        hashAuthors.addData(lastAuthorUtf8);
    }
#ifdef WRITE_RAWDATAFILE
    sourceCode += QStringLiteral("\");\n");
    if (rawDataFile.open(QFile::Append)) {
        QTextStream ts(&rawDataFile);
        ts << sourceCode;
        rawDataFile.close();
        sourceCode.clear();
    }
#else // WRITE_RAWDATAFILE
    QCOMPARE(hashAuthors.result(), currentTestFile.hashAuthors);
#endif // WRITE_RAWDATAFILE

    QCryptographicHash hashFilesUrlsDoi(QCryptographicHash::Md5);
#ifdef WRITE_RAWDATAFILE
    sourceCode = QStringLiteral("static const char *") + filenameStem + QStringLiteral("FilesUrlsDois(\"");
#endif // WRITE_RAWDATAFILE
    for (const QString &filesUrlsDoi : const_cast<const QStringList &>(filesUrlsDoiList)) {
        const QByteArray filesUrlsDoiUtf8 = filesUrlsDoi.toUtf8();
#ifdef WRITE_RAWDATAFILE
        sourceCode += rewriteNonASCII(QString(filesUrlsDoi));
        len += filesUrlsDoiUtf8.length();
        if (len > max_len) {
            sourceCode += QStringLiteral("\"\n        \"");
            len = 0;
        }
#endif // WRITE_RAWDATAFILE
        hashFilesUrlsDoi.addData(filesUrlsDoiUtf8);
    }
#ifdef WRITE_RAWDATAFILE
    sourceCode += QStringLiteral("\");\n");
    if (rawDataFile.open(QFile::Append)) {
        QTextStream ts(&rawDataFile);
        ts << sourceCode;
        rawDataFile.close();
        sourceCode.clear();
    }
#else // WRITE_RAWDATAFILE
    QCOMPARE(hashFilesUrlsDoi.result(), currentTestFile.hashFilesUrlsDoi);
#endif // WRITE_RAWDATAFILE

    delete importer;

    *outFile = bibTeXFile;
}

void KBibTeXFilesTest::saveFile(File *file, const TestFile &currentTestFile, QString *outFile)
{
    *outFile = QString();

    FileExporter *exporter = nullptr;
    if (currentTestFile.filename.endsWith(QStringLiteral(".bib"))) {
        FileExporterBibTeX *bibTeXExporter = new FileExporterBibTeX(this);
        bibTeXExporter->setEncoding(QStringLiteral("utf-8"));
        exporter = bibTeXExporter;
    } else {
        QFAIL(qPrintable(QString::fromLatin1("Don't know format of '%1'").arg(currentTestFile.filename)));
    }

    QTemporaryFile tempFile(QDir::tempPath() + "/XXXXXX." + QFileInfo(currentTestFile.filename).fileName());
    /// It is the function caller's responsibility to remove the temporary file later
    tempFile.setAutoRemove(false);
    QVERIFY(tempFile.open());
    QVERIFY(exporter->save(&tempFile, file));

    *outFile = tempFile.fileName();
}

TestFile KBibTeXFilesTest::createTestFile(const QString &filename, int numElements, int numEntries, const QString &lastEntryId, const QString &lastEntryLastAuthorLastName, const QByteArray &hashAuthors, const QByteArray &hashFilesUrlsDoi)
{
    TestFile r;
    r.filename = filename;
    r.numElements = numElements;
    r.numEntries = numEntries;
    r.lastEntryId = lastEntryId;
    r.lastEntryLastAuthorLastName = lastEntryLastAuthorLastName;
    r.hashAuthors = hashAuthors;
    r.hashFilesUrlsDoi = hashFilesUrlsDoi;
    return r;
}

#ifdef WRITE_RAWDATAFILE
QString KBibTeXFilesTest::rewriteNonASCII(const QString &input)
{
    QString output;
    for (const QChar &c : input) {
        const auto &unicode = c.unicode();
        if (unicode < 128) {
            if (c == QLatin1Char('\n'))
                output.append(QStringLiteral("\\n"));
            else if (c == QLatin1Char('\r'))
                output.append(QStringLiteral("\\r"));
            else if (c == QLatin1Char('\t'))
                output.append(QStringLiteral("\\t"));
            else {
                if (c == QLatin1Char('\\') || c == QLatin1Char('"'))
                    output.append(QLatin1Char('\\'));
                output.append(c);
            }
        } else
            output.append(QString(QStringLiteral("\\u%1")).arg(unicode, 4, 16, QLatin1Char('0')));
    }
    return output;
}
#endif // WRITE_RAWDATAFILE

QTEST_MAIN(KBibTeXFilesTest)

#include "kbibtexfilestest.moc"
