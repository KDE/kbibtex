/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include <qtest_kde.h>

#include <QCryptographicHash>
#include <QTemporaryFile>

#include <KDebug>

#include "entry.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"

typedef struct {
    QString filename;
    int numElements, numEntries;
    QString lastEntryId, lastEntryLastAuthorLastName;
    QByteArray hashAuthors, hashFilesUrlsDoi;
} TestFile;

Q_DECLARE_METATYPE(TestFile);

class KBibTeXFilesTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testFiles_data();
    void testFiles();

private:
    void loadFile(const QString &absoluteFilename, const TestFile &currentTestFile, File **outFile);
    void saveFile(File *file, const TestFile &currentTestFile, QString *outFile);
    TestFile createTestFile(const QString &filename, int numElements, int numEntries, const QString &lastEntryId, const QString &lastEntryLastAuthorLastName, const QString &hashAuthors, const QString &hashFilesUrlsDoi);
};


void KBibTeXFilesTest::initTestCase()
{
    qRegisterMetaType<TestFile>("TestFile");
}

void KBibTeXFilesTest::testFiles_data()
{
    QTest::addColumn<TestFile>("testFile");

    QTest::newRow("bug19489.bib") << createTestFile(QLatin1String("bib/bug19489.bib"), 1, 1, QLatin1String("bart:04:1242"), QLatin1String("Ralph"), QLatin1String("a926264c17545bf6eb9a953a8e3f0970"), QLatin1String("544dc57e2a9908cfb35f35152462ac2e"));
    QTest::newRow("names-with-braces.bib") << createTestFile(QLatin1String("bib/names-with-braces.bib"), 1, 1, QLatin1String("names1"), QLatin1String("{{{{{LastName3A LastName3B}}}}}"), QLatin1String("c4a6575e606ac513b79d1b8e18bca02e"), QLatin1String("d41d8cd98f00b204e9800998ecf8427e"));
    QTest::newRow("duplicates.bib") << createTestFile(QLatin1String("bib/duplicates.bib"), 23, 23, QLatin1String("books/aw/Sedgewick88"), QLatin1String("Sedgewick"), QLatin1String("f743ccb08afda3514a7122710e6590ba"), QLatin1String("9ffb791547bcd10b459dec2d0022314c"));
    QTest::newRow("minix.bib") << createTestFile(QLatin1String("bib/minix.bib"), 163, 123, QLatin1String("Jesshope:2006:ACS"), QLatin1String("Egan"), QLatin1String("c45e76eb809ba4c811f40452215bce04"), QLatin1String("4e64d5806c9be0cf2d77a152157d044c"));
    QTest::newRow("bug19484-refs.bib") << createTestFile(QLatin1String("bib/bug19484-refs.bib"), 641, 641, QLatin1String("Bagnara-etal-2002"), QLatin1String("Hill"), QLatin1String("a6737870e88b13fd2a6dff00b583cc5b"), QLatin1String("71a66fb634275a6f46e5e0828fbceb83"));
    QTest::newRow("bug19362-file15701-database.bib") << createTestFile(QLatin1String("bib/bug19362-file15701-database.bib"), 911, 911, QLatin1String("New1"), QLatin1String("Sunder"), QLatin1String("eb93fa136f4b114c3a6fc4a821b4117c"), QLatin1String("0ac293b3abbfa56867e5514d8bb68614"));
    QTest::newRow("digiplay.bib") << createTestFile(QLatin1String("bib/digiplay.bib"), 3074, 3074, QLatin1String("1180"), QLatin1String("Huizinga"), QLatin1String("2daf695fccb01f4b4cfbe967db62b0a8"), QLatin1String("5d5bf8178652107ab160bc697b5b008f"));
    QTest::newRow("backslash.bib") << createTestFile(QLatin1String("bib/backslash.bib"), 1, 1, QLatin1String("backslash-test"), QLatin1String("Doe"), QLatin1String("4f2ab90ecfa9e5e62cdfb4e55cbb0e05"), QLatin1String("f9f35d6b95b0676751bc613d1d60aa6b"));
}

void KBibTeXFilesTest::testFiles()
{
    QFETCH(TestFile, testFile);

    const QString absoluteFilename = QLatin1String(TESTSET_DIRECTORY) + testFile.filename;
    QVERIFY(QFileInfo(absoluteFilename).exists());

    // first load the file
    File *file = NULL;
    loadFile(absoluteFilename, testFile, &file);
    QVERIFY(file);

    // then save it again to file
    QString tempFileName;
    saveFile(file, testFile, &tempFileName);
    QVERIFY(!tempFileName.isEmpty());

    // try to load again the newly saved version
    File *file2 = NULL;
    loadFile(tempFileName, testFile, &file2);
    QVERIFY(file2);

    QFile::remove(tempFileName);
    delete file;
    delete file2;
}

void KBibTeXFilesTest::loadFile(const QString &absoluteFilename, const TestFile &currentTestFile, File **outFile)
{
    *outFile = NULL;

    FileImporter *importer = NULL;
    if (currentTestFile.filename.endsWith(QLatin1String(".bib"))) {
        importer = new FileImporterBibTeX(false);
    } else {
        QFAIL(qPrintable(QString::fromLatin1("Don't know format of '%1'").arg(currentTestFile.filename)));
    }

    QFile file(absoluteFilename);
    File *bibTeXFile = NULL;
    QVERIFY(file.open(QFile::ReadOnly));
    bibTeXFile = importer->load(&file);
    file.close();

    QVERIFY(bibTeXFile);
    QVERIFY(!bibTeXFile->isEmpty());

    QStringList lastAuthorsList, filesUrlsDoiList;
    int countElements = bibTeXFile->count(), countEntries = 0;
    QString lastEntryId, lastEntryLastAuthorLastName;
    for (File::ConstIterator fit = bibTeXFile->constBegin(); fit != bibTeXFile->constEnd(); ++fit) {
        Element *element = (*fit).data();
        Entry *entry = dynamic_cast<Entry *>(element);
        if (entry != NULL) {
            ++countEntries;
            lastEntryId = entry->id();

            Value authors = entry->value(Entry::ftAuthor);
            if (!authors.isEmpty()) {
                ValueItem *vi = authors.last().data();
                Person *p = dynamic_cast<Person *>(vi);
                if (p != NULL) {
                    lastEntryLastAuthorLastName = p->lastName();
                } else
                    lastEntryLastAuthorLastName.clear();
            } else {
                Value editors = entry->value(Entry::ftEditor);
                if (!editors.isEmpty()) {
                    ValueItem *vi = editors.last().data();
                    Person *p = dynamic_cast<Person *>(vi);
                    if (p != NULL) {
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
                const QString field = index == 1 ? Entry::ftUrl : QString(QLatin1String("%1%2")).arg(Entry::ftUrl).arg(index);
                Value v = entry->value(field);
                foreach(const QSharedPointer<ValueItem> &vi, v) {
                    filesUrlsDoiList << PlainTextValue::text(vi);
                }
                if (v.isEmpty() && index > 10) break;
            }
            for (int index = 1; index < 100; ++index) {
                const QString field = index == 1 ? Entry::ftDOI : QString(QLatin1String("%1%2")).arg(Entry::ftDOI).arg(index);
                Value v = entry->value(field);
                foreach(const QSharedPointer<ValueItem> &vi, v) {
                    filesUrlsDoiList << PlainTextValue::text(vi);
                }
                if (v.isEmpty() && index > 10) break;
            }
            for (int index = 1; index < 100; ++index) {
                const QString field = index == 1 ? Entry::ftLocalFile : QString(QLatin1String("%1%2")).arg(Entry::ftLocalFile).arg(index);
                Value v = entry->value(field);
                foreach(const QSharedPointer<ValueItem> &vi, v) {
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

    QCryptographicHash hashAuthors(QCryptographicHash::Md4);
    lastAuthorsList.sort();
    foreach (const QString &it, lastAuthorsList)
        hashAuthors.addData(it.toUtf8());
    QCOMPARE(hashAuthors.result(), currentTestFile.hashAuthors);

    QCryptographicHash hashFilesUrlsDoi(QCryptographicHash::Md5);
    foreach (const QString &it, filesUrlsDoiList)
        hashFilesUrlsDoi.addData(it.toUtf8());
    QCOMPARE(hashFilesUrlsDoi.result(), currentTestFile.hashFilesUrlsDoi);

    delete importer;

    *outFile = bibTeXFile;
}

void KBibTeXFilesTest::saveFile(File *file, const TestFile &currentTestFile, QString *outFile)
{
    *outFile = QString();

    FileExporter *exporter = NULL;
    if (currentTestFile.filename.endsWith(QLatin1String(".bib"))) {
        FileExporterBibTeX *bibTeXExporter = new FileExporterBibTeX();
        bibTeXExporter->setEncoding(QLatin1String("utf-8"));
        exporter = bibTeXExporter;
    } else {
        QFAIL(qPrintable(QString::fromLatin1("Don't know format of '%1'").arg(currentTestFile.filename)));
    }

    QTemporaryFile tempFile(QDir::tempPath() + "/XXXXXX." + QFileInfo(currentTestFile.filename).fileName());
    // will be removed manually
    tempFile.setAutoRemove(false);
    QVERIFY(tempFile.open());
    QVERIFY(exporter->save(&tempFile, file));

    *outFile = tempFile.fileName();
}

TestFile KBibTeXFilesTest::createTestFile(const QString &filename, int numElements, int numEntries, const QString &lastEntryId, const QString &lastEntryLastAuthorLastName, const QString &hashAuthors, const QString &hashFilesUrlsDoi)
{
    TestFile r;
    r.filename = filename;
    r.numElements = numElements;
    r.numEntries = numEntries;
    r.lastEntryId = lastEntryId;
    r.lastEntryLastAuthorLastName = lastEntryLastAuthorLastName;
    r.hashAuthors = QByteArray::fromHex(hashAuthors.toLatin1());
    r.hashFilesUrlsDoi = QByteArray::fromHex(hashFilesUrlsDoi.toLatin1());
    return r;
}

QTEST_KDEMAIN_CORE(KBibTeXFilesTest)

#include "kbibtextfilestest.moc"
