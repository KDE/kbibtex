#include <KApplication>
#include <KIcon>
#include <KPushButton>
#include <KListWidget>
#include <KStandardDirs>
#include <KAction>
#include <KDebug>

#include <QProgressBar>
#include <QTimer>
#include <QLayout>
#include <QFileInfo>
#include <QMenu>
#include <QCryptographicHash>
#include <QSignalMapper>
#include <QProgressBar>

#include <onlinesearchacmportal.h>
#include <onlinesearcharxiv.h>
#include <onlinesearchbibsonomy.h>
#include <onlinesearchgooglescholar.h>
#include <onlinesearchieeexplore.h>
#include <onlinesearchingentaconnect.h>
#include <onlinesearchjstor.h>
#include <onlinesearchmathscinet.h>
#include <onlinesearchpubmed.h>
#include <onlinesearchsciencedirect.h>
#include <onlinesearchspringerlink.h>
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"
#include "kbibtextest.h"
#include "version.h"

KIcon iconOK(QLatin1String("dialog-ok-apply"));
KIcon iconERROR(QLatin1String("dialog-cancel"));
KIcon iconINFO(QLatin1String("dialog-information"));
KIcon iconAUTH(QLatin1String("dialog-password"), NULL, QStringList() << QLatin1String("dialog-cancel"));
KIcon iconNETWORK(QLatin1String("network-wired"), NULL, QStringList() << QLatin1String("dialog-cancel"));

int filenameCounter = 0;

class TestWidget : public QWidget
{
private:
    KBibTeXTest *m_parent;
    KPushButton *buttonStartTest;
    QProgressBar *progressBar;
    KAction *actionStartOnlineSearchTests, *actionStartAllTestFileTests;

public:
    KListWidget *messageList;

    TestWidget(KBibTeXTest *parent)
            : QWidget(parent), m_parent(parent) {
        QGridLayout *layout = new QGridLayout(this);

        buttonStartTest = new KPushButton(KIcon("application-x-executable"), QLatin1String("Start Tests"), this);
        layout->addWidget(buttonStartTest, 0, 0, 1, 1);

        progressBar = new QProgressBar(this);
        layout->addWidget(progressBar, 0, 1, 1, 3);
        progressBar->setVisible(false);

        messageList = new KListWidget(this);
        layout->addWidget(messageList, 1, 0, 4, 4);
    }

    void setProgress(int pos, int total) {
        if (pos < 0 || total < 0)
            progressBar->setVisible(false);
        else {
            progressBar->setVisible(true);
            progressBar->setMaximum(total);
            progressBar->setValue(pos);
        }
    }

    void setupMenus() {
        QMenu *menu = new QMenu(buttonStartTest);
        buttonStartTest->setMenu(menu);

        /// ** Online Search **
        actionStartOnlineSearchTests = new KAction(QLatin1String("Online Search"), m_parent);
        connect(actionStartOnlineSearchTests, SIGNAL(triggered()), m_parent, SLOT(startOnlineSearchTests()));
        menu->addAction(actionStartOnlineSearchTests);

        /// ** File Import/Export **
        actionStartAllTestFileTests = new KAction(QLatin1String("All Tests"), m_parent);
        connect(actionStartAllTestFileTests, SIGNAL(triggered()), m_parent, SLOT(startAllTestFileTests()));

        QMenu *subMenu = menu->addMenu("File Import/Export");
        subMenu->addAction(actionStartAllTestFileTests);

        subMenu->addSeparator();
        QSignalMapper *fileTestsSignalMapper = new QSignalMapper(subMenu);
        connect(fileTestsSignalMapper, SIGNAL(mapped(int)), m_parent, SLOT(startTestFileTest(int)));
        int id = 0;
        for (QList<KBibTeXTest::TestFile *>::ConstIterator it = m_parent->testFiles.constBegin(); it != m_parent->testFiles.constEnd(); ++it, ++id) {
            KBibTeXTest::TestFile *testFile = *it;
            const QString label = QFileInfo(testFile->filename).fileName();
            QAction *action = subMenu->addAction(label, fileTestsSignalMapper, SLOT(map()));
            fileTestsSignalMapper->setMapping(action, id);
        }
    }

    void setBusy(bool isBusy) {
        buttonStartTest->setEnabled(!isBusy);
        actionStartOnlineSearchTests->setEnabled(!isBusy);
        actionStartAllTestFileTests->setEnabled(!isBusy);
    }
};

KBibTeXTest::KBibTeXTest(QWidget *parent)
        : KDialog(parent), m_running(false), m_isBusy(false)
{
    testFiles << createTestFile(QLatin1String("bib/bug19489.bib"), 1, 1, QLatin1String("bart:04:1242"), QLatin1String("Ralph"), QLatin1String("a926264c17545bf6eb9a953a8e3f0970"));
    testFiles << createTestFile(QLatin1String("bib/names-with-braces.bib"), 1, 1, QLatin1String("names1"), QLatin1String("{{{{{LastName3A LastName3B}}}}}"), QLatin1String("c4a6575e606ac513b79d1b8e18bca02e"));
    testFiles << createTestFile(QLatin1String("bib/duplicates.bib"), 23, 23, QLatin1String("books/aw/Sedgewick88"), QLatin1String("Sedgewick"), QLatin1String("f743ccb08afda3514a7122710e6590ba"));
    testFiles << createTestFile(QLatin1String("bib/minix.bib"), 163, 123, QLatin1String("Jesshope:2006:ACS"), QLatin1String("Egan"), QLatin1String("c45e76eb809ba4c811f40452215bce04"));
    testFiles << createTestFile(QLatin1String("bib/bug19484-refs.bib"), 641, 641, QLatin1String("Bagnara-etal-2002"), QLatin1String("Hill"), QLatin1String("a6737870e88b13fd2a6dff00b583cc5b"));
    testFiles << createTestFile(QLatin1String("bib/bug19362-file15701-database.bib"), 911, 911, QLatin1String("New1"), QLatin1String("Sunder"), QLatin1String("eb93fa136f4b114c3a6fc4a821b4117c"));
    testFiles << createTestFile(QLatin1String("bib/digiplay.bib"), 3074, 3074, QLatin1String("1180"), QLatin1String("Huizinga"), QLatin1String("2daf695fccb01f4b4cfbe967db62b0a8"));
    m_onlineSearchList << new OnlineSearchAcmPortal(this);
    m_onlineSearchList << new OnlineSearchArXiv(this);
    m_onlineSearchList << new OnlineSearchBibsonomy(this);
    m_onlineSearchList << new OnlineSearchGoogleScholar(this);
    m_onlineSearchList << new OnlineSearchIEEEXplore(this);
    m_onlineSearchList << new OnlineSearchIngentaConnect(this);
    m_onlineSearchList << new OnlineSearchJStor(this);
    m_onlineSearchList << new OnlineSearchMathSciNet(this);
    m_onlineSearchList << new OnlineSearchPubMed(this);
    m_onlineSearchList << new OnlineSearchScienceDirect(this);
    m_onlineSearchList << new OnlineSearchSpringerLink(this);
    m_currentOnlineSearch = m_onlineSearchList.constBegin();

    setPlainCaption(QLatin1String("KBibTeX Test Suite"));
    setButtons(KDialog::Close);

    m_testWidget = new TestWidget(this);
    const int fontSize = m_testWidget->fontMetrics().width(QLatin1Char('a'));
    m_testWidget->setMinimumSize(fontSize * 96, fontSize * 48);
    setMainWidget(m_testWidget);
    m_testWidget->setupMenus();

    connect(this, SIGNAL(closeClicked()), this, SLOT(aboutToQuit()));

    addMessage(QString(QLatin1String("Compiled for %1")).arg(versionNumber), iconINFO);
}

void KBibTeXTest::addMessage(const QString &message, const KIcon &icon)
{
    QListWidgetItem *item = icon.isNull() ? new QListWidgetItem(message) : new QListWidgetItem(icon, message);
    item->setToolTip(item->text());
    m_testWidget->messageList->addItem(item);
    m_testWidget->messageList->scrollToBottom();
    kapp->processEvents();
}

void KBibTeXTest::setBusy(bool isBusy)
{
    m_testWidget->setBusy(isBusy);
    if (isBusy && !m_isBusy) {
        /// changing to busy state
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    } else if (!isBusy && m_isBusy) {
        /// changing to idle state
        QApplication::restoreOverrideCursor();
    }
    m_isBusy = isBusy;
}

void KBibTeXTest::aboutToQuit()
{
    m_running = false;
    QTimer::singleShot(500, kapp, SLOT(quit()));
}

void KBibTeXTest::startOnlineSearchTests()
{
    m_running = true;
    setBusy(true);
    m_testWidget->messageList->clear();
    kapp->processEvents();
    processNextSearch();
}

void KBibTeXTest::onlineSearchStoppedSearch(int searchResult)
{
    if (searchResult == OnlineSearchAbstract::resultNoError) {
        if (m_currentOnlineSearchNumFoundEntries == 0)
            addMessage(QString(QLatin1String("Got no error message searching '%1', but found NO entries")).arg((*m_currentOnlineSearch)->label()), iconERROR);
        else
            addMessage(QString(QLatin1String("No error searching '%1', found %2 entries")).arg((*m_currentOnlineSearch)->label()).arg(m_currentOnlineSearchNumFoundEntries), iconOK);
    } else if (searchResult == OnlineSearchAbstract::resultAuthorizationRequired) {
        addMessage(QString(QLatin1String("Authorization required for '%1'")).arg((*m_currentOnlineSearch)->label()), iconAUTH);
    } else if (searchResult == OnlineSearchAbstract::resultNetworkError) {
        addMessage(QString(QLatin1String("Network error for '%1'")).arg((*m_currentOnlineSearch)->label()), iconNETWORK);
    } else {
        addMessage(QString(QLatin1String("Error searching '%1'")).arg((*m_currentOnlineSearch)->label()), iconERROR);
    }
    m_currentOnlineSearch++;

    processNextSearch();
}

void KBibTeXTest::onlineSearchFoundEntry()
{
    ++m_currentOnlineSearchNumFoundEntries;
}

void KBibTeXTest::progress(int pos, int total)
{
    m_testWidget->setProgress(pos, total);
}

void KBibTeXTest::processNextSearch()
{
    if (m_running && m_currentOnlineSearch != m_onlineSearchList.constEnd()) {
        setBusy(true);
        m_currentOnlineSearchNumFoundEntries = 0;
        addMessage(QString(QLatin1String("Searching '%1'")).arg((*m_currentOnlineSearch)->label()), iconINFO);

        QMap<QString, QString> query;
        query.insert(OnlineSearchAbstract::queryKeyAuthor, QLatin1String("smith"));
        connect((*m_currentOnlineSearch), SIGNAL(stoppedSearch(int)), this, SLOT(onlineSearchStoppedSearch(int)));
        connect((*m_currentOnlineSearch), SIGNAL(foundEntry(QSharedPointer<Entry>)), this, SLOT(onlineSearchFoundEntry()));
        (*m_currentOnlineSearch)->startSearch(query, 3);
    } else {
        addMessage(QLatin1String("Done testing"), iconINFO);
        setBusy(false);
        m_running = false;
    }
}

void KBibTeXTest::startAllTestFileTests()
{
    m_running = true;
    setBusy(true);
    m_testWidget->messageList->clear();
    kapp->processEvents();

    for (QList<TestFile *>::ConstIterator it = testFiles.constBegin(); it != testFiles.constEnd(); ++it) {
        processFileTest(*it);
    }

    addMessage(QLatin1String("Done testing"), iconINFO);
    setBusy(false);
    m_running = false;
}

void KBibTeXTest::startTestFileTest(int index)
{
    m_running = true;
    setBusy(true);
    m_testWidget->messageList->clear();
    kapp->processEvents();

    TestFile *testFile = testFiles[index];
    processFileTest(testFile);

    addMessage(QLatin1String("Done testing"), iconINFO);
    setBusy(false);
    m_running = false;
}

void KBibTeXTest::processFileTest(TestFile *testFile)
{
    kapp->processEvents();

    const QString absoluteFilename = QLatin1String(TESTSET_DIRECTORY) + testFile->filename;
    if (!QFileInfo(absoluteFilename).exists()) {
        addMessage(QString(QLatin1String("File '%1' does not exist")).arg(absoluteFilename), iconERROR);
        return;
    }

    File *file = loadFile(absoluteFilename, testFile);
    if (file == NULL)
        return;
    const QString tempFileName = saveFile(file, testFile);
    if (tempFileName.isEmpty()) {
        delete file;
        return;
    }
    File *file2 = loadFile(tempFileName, testFile);
    if (file2 == NULL) {
        delete file;
        return;
    }

    delete file;
    delete file2;
}

File *KBibTeXTest::loadFile(const QString &absoluteFilename, TestFile *currentTestFile)
{
    progress(-1, -1);

    FileImporter *importer = NULL;
    if (currentTestFile->filename.endsWith(QLatin1String(".bib"))) {
        importer = new FileImporterBibTeX(false);
        connect(importer, SIGNAL(progress(int, int)), this, SLOT(progress(int, int)));
    } else {
        addMessage(QString(QLatin1String("Don't know format of '%1'")).arg(QFileInfo(absoluteFilename).fileName()), iconERROR);
        return NULL;
    }

    addMessage(QString(QLatin1String("Loading file '%1'")).arg(QFileInfo(absoluteFilename).fileName()), iconINFO);
    QFile file(absoluteFilename);
    File *bibTeXFile = NULL;
    if (file.open(QFile::ReadOnly)) {
        bibTeXFile = importer->load(&file);
        progress(-1, -1);
        file.close();
    } else {
        addMessage(QString(QLatin1String("Cannot open file '%1'")).arg(absoluteFilename), iconERROR);
        delete importer;
        return NULL;
    }

    if (bibTeXFile == NULL || bibTeXFile->isEmpty()) {
        addMessage(QString(QLatin1String("File '%1' seems to be empty")).arg(QFileInfo(absoluteFilename).fileName()), iconERROR);
        delete importer;
        if (bibTeXFile != NULL) delete bibTeXFile;
        return NULL;
    }

    QStringList lastAuthorsList;
    int countElements = bibTeXFile->count(), countEntries = 0;
    QString lastEntryId, lastEntryLastAuthorLastName;
    for (File::ConstIterator fit = bibTeXFile->constBegin(); fit != bibTeXFile->constEnd(); ++fit) {
        Element *element = (*fit).data();
        Entry *entry = dynamic_cast<Entry *>(element);
        if (entry != NULL) {
            ++countEntries;
            lastEntryId = entry->id();

            Value authors = entry->value(Entry::ftAuthor);
            if (!authors.empty()) {
                ValueItem *vi = authors.last().data();
                Person *p = dynamic_cast<Person *>(vi);
                if (p != NULL) {
                    lastEntryLastAuthorLastName = p->lastName();
                } else
                    lastEntryLastAuthorLastName = QString::null;
            } else {
                Value editors = entry->value(Entry::ftEditor);
                if (!editors.empty()) {
                    ValueItem *vi = editors.last().data();
                    Person *p = dynamic_cast<Person *>(vi);
                    if (p != NULL) {
                        lastEntryLastAuthorLastName = p->lastName();
                    } else
                        lastEntryLastAuthorLastName = QString::null;
                } else
                    lastEntryLastAuthorLastName = QString::null;
            }

            if (!lastEntryLastAuthorLastName.isEmpty()) {
                if (lastEntryLastAuthorLastName[0] == QLatin1Char('{') && lastEntryLastAuthorLastName[lastEntryLastAuthorLastName.length() - 1] == QLatin1Char('}'))
                    lastEntryLastAuthorLastName = lastEntryLastAuthorLastName.mid(1, lastEntryLastAuthorLastName.length() - 2);
                lastAuthorsList << lastEntryLastAuthorLastName;
            }
        }
    }

    if (countElements != currentTestFile->numElements) {
        addMessage(QString(QLatin1String("File '%1' is supposed to have %2 elements, but %3 were counted")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->numElements).arg(countElements), iconERROR);
        QSharedPointer<Entry> entry = bibTeXFile->at(bibTeXFile->count() - 1).dynamicCast<Entry>();
        if (!entry.isNull())
            kDebug() << "Last entry had id" << entry->id() << "(expected:" << currentTestFile->lastEntryId << ")";
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else if (countEntries != currentTestFile->numEntries) {
        addMessage(QString(QLatin1String("File '%1' is supposed to have %2 entries, but %3 were counted")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->numEntries).arg(countEntries), iconERROR);
        QSharedPointer<Entry> entry = bibTeXFile->at(bibTeXFile->count() - 1).dynamicCast<Entry>();
        if (!entry.isNull())
            kDebug() << "Last entry had id" << entry->id() << "(expected:" << currentTestFile->lastEntryId << ")";
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else if (lastEntryId != currentTestFile->lastEntryId) {
        addMessage(QString(QLatin1String("File '%1' is supposed to have '%2' as last entry id, but '%3' was found instead")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->lastEntryId).arg(lastEntryId), iconERROR);
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else if (lastEntryLastAuthorLastName != currentTestFile->lastEntryLastAuthorLastName) {
        addMessage(QString(QLatin1String("File '%1' is supposed to have '%2' as last entry's last author, but '%3' was found instead")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->lastEntryLastAuthorLastName).arg(lastEntryLastAuthorLastName), iconERROR);
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else {
        /// Delay cryptographic hash computation until no other test has failed
        QCryptographicHash hash(QCryptographicHash::Md4);
        lastAuthorsList.sort();
        for (QStringList::ConstIterator it = lastAuthorsList.constBegin(); it != lastAuthorsList.constEnd(); ++it)
            hash.addData((*it).toUtf8());

        const QString authorListFilename = KStandardDirs::locateLocal("tmp", QFileInfo(currentTestFile->filename).baseName()) + QString(QLatin1String("_authors_%1.txt")).arg(++filenameCounter);
        QFile authorListFile(authorListFilename);
        if (authorListFile.open(QFile::WriteOnly)) {
            QTextStream ts(&authorListFile);
            for (QStringList::ConstIterator it = lastAuthorsList.constBegin(); it != lastAuthorsList.constEnd(); ++it)
                ts << *it << endl;
            authorListFile.close();
            kDebug() << "Saved list of authors in" << authorListFilename;
        } else
            kDebug() << "Failed to write list of authors to" << authorListFilename;

        if (hash.result() != currentTestFile->md4sum) {
            kDebug() << hash.result().toHex().data() << "for" << absoluteFilename << "based on" << currentTestFile->filename;
            addMessage(QString(QLatin1String("A hash sum over all last authors in file '%1' did not match the expected hash sum (%2)")).arg(QFileInfo(absoluteFilename).fileName()).arg(hash.result().toHex().data()), iconERROR);
            delete importer;
            delete bibTeXFile;
            return NULL;
        } else {
            addMessage(QString(QLatin1String("File '%1' was imported correctly")).arg(QFileInfo(absoluteFilename).fileName()), iconOK);
        }
    }

    delete importer;

    return bibTeXFile;
}

QString KBibTeXTest::saveFile(File *file, TestFile *currentTestFile)
{
    progress(-1, -1);

    const QString tempFilename = KStandardDirs::locateLocal("tmp", QFileInfo(currentTestFile->filename).fileName());

    FileExporter *exporter = NULL;
    if (currentTestFile->filename.endsWith(QLatin1String(".bib"))) {
        exporter = new FileExporterBibTeX();
        connect(exporter, SIGNAL(progress(int, int)), this, SLOT(progress(int, int)));
    } else {
        addMessage(QString(QLatin1String("Don't know format of '%1'")).arg(tempFilename), iconERROR);
        return NULL;
    }

    QFile tempFile(tempFilename);
    if (tempFile.open(QFile::WriteOnly)) {
        bool result = exporter->save(&tempFile, file);
        tempFile.close();
        progress(-1, -1);
        if (!result)    {
            addMessage(QString(QLatin1String("Could save to temporary file '%1'")).arg(QFileInfo(tempFile.fileName()).fileName()), iconERROR);
            delete exporter;
            return QString::null;
        }
    } else {
        addMessage(QString(QLatin1String("Could not open temporary file '%1'")).arg(QFileInfo(tempFile.fileName()).fileName()), iconERROR);
        delete exporter;
        return QString::null;
    }

    addMessage(QString(QLatin1String("File '%1' was exported to '%2'")).arg(QFileInfo(currentTestFile->filename).fileName()).arg(tempFilename), iconOK);
    return tempFilename;
}

KBibTeXTest::TestFile *KBibTeXTest::createTestFile(const QString &filename, int numElements, int numEntries, const QString &lastEntryId, const QString &lastEntryLastAuthorLastName, const QString &md4sumHex)
{
    TestFile *r = new TestFile();
    r->filename = filename;
    r->numElements = numElements;
    r->numEntries = numEntries;
    r->lastEntryId = lastEntryId;
    r->lastEntryLastAuthorLastName = lastEntryLastAuthorLastName;
    r->md4sum = QByteArray::fromHex(md4sumHex.toAscii());
    return r;
}
