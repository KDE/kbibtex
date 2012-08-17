#include <KApplication>
#include <KIcon>
#include <KPushButton>
#include <KListWidget>
#include <KTemporaryFile>
#include <KAction>
#include <KDebug>

#include <QProgressBar>
#include <QTimer>
#include <QLayout>
#include <QFileInfo>
#include <QMenu>
#include <QCryptographicHash>

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
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <file.h>
#include "kbibtextest.h"

KIcon iconOK(QLatin1String("dialog-ok-apply"));
KIcon iconERROR(QLatin1String("dialog-cancel"));
KIcon iconINFO(QLatin1String("dialog-information"));
KIcon iconAUTH(QLatin1String("dialog-password"), NULL, QStringList() << QLatin1String("dialog-cancel"));
KIcon iconNETWORK(QLatin1String("network-wired"), NULL, QStringList() << QLatin1String("dialog-cancel"));

int filenameCounter = 0;

class TestWidget : public QWidget
{
public:
    KListWidget *messageList;
    KPushButton *buttonStartTest;
    KAction *actionStartOnlineSearchTests, *actionStartTestFileTests;

    TestWidget(KBibTeXTest *parent)
            : QWidget(parent) {
        actionStartOnlineSearchTests = new KAction(QLatin1String("Online Search"), parent);
        connect(actionStartOnlineSearchTests, SIGNAL(triggered()), parent, SLOT(startOnlineSearchTests()));
        actionStartTestFileTests = new KAction(QLatin1String("File Import/Export"), parent);
        connect(actionStartTestFileTests, SIGNAL(triggered()), parent, SLOT(startTestFileTests()));

        QGridLayout *layout = new QGridLayout(this);

        buttonStartTest = new KPushButton(KIcon("application-x-executable"), QLatin1String("Start Tests"), this);
        layout->addWidget(buttonStartTest, 0, 0, 1, 1);
        QMenu *menu = new QMenu(buttonStartTest);
        menu->addAction(actionStartOnlineSearchTests);
        menu->addAction(actionStartTestFileTests);
        buttonStartTest->setMenu(menu);

        messageList = new KListWidget(this);
        layout->addWidget(messageList, 1, 0, 4, 4);
    }
};

KBibTeXTest::KBibTeXTest(QWidget *parent)
        : KDialog(parent), m_running(false), m_currentOnlineSearch(NULL)
{
    setPlainCaption(QLatin1String("KBibTeX Test Suite"));
    setButtons(KDialog::Close);

    m_testWidget = new TestWidget(this);
    const int fontSize = m_testWidget->fontMetrics().width(QLatin1Char('a'));
    m_testWidget->setMinimumSize(fontSize * 96, fontSize * 48);
    setMainWidget(m_testWidget);

    connect(this, SIGNAL(closeClicked()), this, SLOT(aboutToQuit()));
}

void KBibTeXTest::addMessage(const QString &message, const KIcon &icon)
{
    QListWidgetItem *item = icon.isNull() ? new QListWidgetItem(message) : new QListWidgetItem(icon, message);
    m_testWidget->messageList->addItem(item);
    m_testWidget->messageList->scrollToBottom();
    kapp->processEvents();
}

void KBibTeXTest::aboutToQuit()
{
    m_running = false;
    QTimer::singleShot(500, kapp, SLOT(quit()));
}

void KBibTeXTest::startOnlineSearchTests()
{
    m_running = true;
    m_testWidget->buttonStartTest->setEnabled(false);
    m_testWidget->messageList->clear();
    kapp->processEvents();

    m_onlineSearchList.clear();
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

    processNextSearch();
}

void KBibTeXTest::onlineSearchStoppedSearch(int searchResult)
{
    if (searchResult == OnlineSearchAbstract::resultNoError) {
        addMessage(QString(QLatin1String("No error searching \"%1\", found %2 entries")).arg(m_currentOnlineSearch->label()).arg(m_currentOnlineSearchNumFoundEntries), iconOK);
    } else if (searchResult == OnlineSearchAbstract::resultAuthorizationRequired) {
        addMessage(QString(QLatin1String("Authorization required for \"%1\"")).arg(m_currentOnlineSearch->label()), iconAUTH);
    } else if (searchResult == OnlineSearchAbstract::resultNetworkError) {
        addMessage(QString(QLatin1String("Network error for \"%1\"")).arg(m_currentOnlineSearch->label()), iconNETWORK);
    } else {
        addMessage(QString(QLatin1String("Error searching \"%1\"")).arg(m_currentOnlineSearch->label()), iconERROR);
    }

    processNextSearch();
}

void KBibTeXTest::onlineSearchFoundEntry()
{
    ++m_currentOnlineSearchNumFoundEntries;
}

void KBibTeXTest::processNextSearch()
{
    if (m_running && !m_onlineSearchList.isEmpty()) {
        m_currentOnlineSearchNumFoundEntries = 0;
        m_currentOnlineSearch = m_onlineSearchList.first();
        m_onlineSearchList.removeFirst();
        addMessage(QString(QLatin1String("Searching \"%1\"")).arg(m_currentOnlineSearch->label()), iconINFO);

        QMap<QString, QString> query;
        query.insert(OnlineSearchAbstract::queryKeyAuthor, QLatin1String("smith"));
        connect(m_currentOnlineSearch, SIGNAL(stoppedSearch(int)), this, SLOT(onlineSearchStoppedSearch(int)));
        connect(m_currentOnlineSearch, SIGNAL(foundEntry(QSharedPointer<Entry>)), this, SLOT(onlineSearchFoundEntry()));
        m_currentOnlineSearch->startSearch(query, 3);
    } else {
        addMessage(QLatin1String("Done testing"), iconINFO);
        m_testWidget->buttonStartTest->setEnabled(true);
        m_running = false;
    }
}

void KBibTeXTest::startTestFileTests()
{
    m_running = true;
    m_testWidget->buttonStartTest->setEnabled(false);
    m_testWidget->messageList->clear();
    kapp->processEvents();

    QList<TestFile *> testFiles = QList<TestFile *>() << createTestFile(QLatin1String("bib/bug19489.bib"), 1, 1, QLatin1String("bart:04:1242"), QLatin1String("Ralph"), QLatin1String("a926264c17545bf6eb9a953a8e3f0970"));
    testFiles << createTestFile(QLatin1String("bib/names-with-braces.bib"), 1, 1, QLatin1String("names1"), QLatin1String("LastName3A LastName3B"), QLatin1String("e0cf18bc193f545e576d128106deafbb"));
    testFiles << createTestFile(QLatin1String("bib/duplicates.bib"), 23, 23, QLatin1String("books/aw/Sedgewick88"), QLatin1String("Sedgewick"), QLatin1String("f743ccb08afda3514a7122710e6590ba"));
    testFiles << createTestFile(QLatin1String("bib/minix.bib"), 163, 123, QLatin1String("Jesshope:2006:ACS"), QLatin1String("Egan"), QLatin1String("c45e76eb809ba4c811f40452215bce04"));
    testFiles << createTestFile(QLatin1String("bib/bug19484-refs.bib"), 641, 641, QLatin1String("Bagnara-etal-2002"), QLatin1String("Hill"), QLatin1String("a6737870e88b13fd2a6dff00b583cc5b"));
    testFiles << createTestFile(QLatin1String("bib/bug19362-file15701-database.bib"), 911, 911, QLatin1String("New1"), QLatin1String("Sunder"), QLatin1String("eb93fa136f4b114c3a6fc4a821b4117c"));
    testFiles << createTestFile(QLatin1String("bib/digiplay.bib"), 3074, 3074, QLatin1String("1180"), QLatin1String("Huizinga"), QLatin1String("2daf695fccb01f4b4cfbe967db62b0a8"));

    m_running = true;
    for (QList<TestFile *>::ConstIterator it = testFiles.constBegin(); it != testFiles.constEnd(); ++it) {
        TestFile *currentTestFile = *it;

        const QString absoluteFilename = QLatin1String(TESTSET_DIRECTORY) + currentTestFile->filename;
        if (!QFileInfo(absoluteFilename).exists()) {
            addMessage(QString(QLatin1String("File \"%1\" does not exist")).arg(absoluteFilename), iconERROR);
            continue;
        }

        File *file = loadFile(absoluteFilename, currentTestFile);
        if (file == NULL)
            continue;
        QString tempFileName = saveFile(file, currentTestFile);
        if (tempFileName.isEmpty())
            continue;
        File *file2 = loadFile(tempFileName, currentTestFile);
        if (file2 == NULL) {
            delete file;
            continue;
        }

        delete file;
        delete file2;
    }

    addMessage(QLatin1String("Done testing"), iconINFO);
    m_testWidget->buttonStartTest->setEnabled(true);
    m_running = false;
}

File *KBibTeXTest::loadFile(const QString &absoluteFilename, TestFile *currentTestFile)
{
    FileImporter *importer = NULL;
    if (currentTestFile->filename.endsWith(QLatin1String(".bib")))
        importer = new FileImporterBibTeX(false);
    else {
        addMessage(QString(QLatin1String("Don't know format of \"%1\"")).arg(QFileInfo(absoluteFilename).fileName()), iconERROR);
        return NULL;
    }

    addMessage(QString(QLatin1String("Loading file \"%1\"")).arg(QFileInfo(absoluteFilename).fileName()), iconINFO);
    QFile file(absoluteFilename);
    File *bibTeXFile = NULL;
    if (file.open(QFile::ReadOnly)) {
        bibTeXFile = importer->load(&file);
        file.close();
    } else {
        addMessage(QString(QLatin1String("Cannot open file \"%1\"")).arg(absoluteFilename), iconERROR);
        delete importer;
        return NULL;
    }

    if (bibTeXFile == NULL || bibTeXFile->isEmpty()) {
        addMessage(QString(QLatin1String("File \"%1\" seems to be empty")).arg(QFileInfo(absoluteFilename).fileName()), iconERROR);
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
                if (lastEntryLastAuthorLastName[0] == QChar('{') && lastEntryLastAuthorLastName[lastEntryLastAuthorLastName.length() - 1] == QChar('}'))
                    lastEntryLastAuthorLastName = lastEntryLastAuthorLastName.mid(1, lastEntryLastAuthorLastName.length() - 2);
                lastAuthorsList << lastEntryLastAuthorLastName;
            }
        }
    }

    if (countElements != currentTestFile->numElements) {
        addMessage(QString(QLatin1String("File \"%1\" is supposed to have %2 elements, but only %3 were counted")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->numElements).arg(countElements), iconERROR);
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else if (countEntries != currentTestFile->numEntries) {
        addMessage(QString(QLatin1String("File \"%1\" is supposed to have %2 entries, but only %3 were counted")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->numEntries).arg(countEntries), iconERROR);
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else if (lastEntryId != currentTestFile->lastEntryId) {
        addMessage(QString(QLatin1String("File \"%1\" is supposed to have \"%2\" as last entry id, but \"%3\" was found instead")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->lastEntryId).arg(lastEntryId), iconERROR);
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else if (lastEntryLastAuthorLastName != currentTestFile->lastEntryLastAuthorLastName) {
        addMessage(QString(QLatin1String("File \"%1\" is supposed to have \"%2\" as last entry's last author, but \"%3\" was found instead")).arg(QFileInfo(absoluteFilename).fileName()).arg(currentTestFile->lastEntryLastAuthorLastName).arg(lastEntryLastAuthorLastName), iconERROR);
        delete importer;
        delete bibTeXFile;
        return NULL;
    } else {
        /// Delay cryptographic hash computation until no other test has failed
        QCryptographicHash hash(QCryptographicHash::Md4);
        lastAuthorsList.sort();
        for (QStringList::ConstIterator it = lastAuthorsList.constBegin(); it != lastAuthorsList.constEnd(); ++it)
            hash.addData((*it).toUtf8());

        if (hash.result() != currentTestFile->md4sum) {
            kDebug() << hash.result().toHex().data() << "for" << absoluteFilename << "based on" << currentTestFile->filename;
            addMessage(QString(QLatin1String("A hash sum over all last authors in file \"%1\" did not match the expected hash sum (%2)")).arg(QFileInfo(absoluteFilename).fileName()).arg(hash.result().toHex().data()), iconERROR);
            delete importer;
            delete bibTeXFile;
            return NULL;
        } else
            addMessage(QString(QLatin1String("File \"%1\" was imported correctly")).arg(QFileInfo(absoluteFilename).fileName()), iconOK);
    }

    delete importer;

    return bibTeXFile;
}

QString KBibTeXTest::saveFile(File *file, TestFile *currentTestFile)
{
    KTemporaryFile tempFile;
    tempFile.setAutoRemove(false);

    FileExporter *exporter = NULL;
    if (currentTestFile->filename.endsWith(QLatin1String(".bib"))) {
        exporter = new FileExporterBibTeX();
        tempFile.setSuffix(QLatin1String(".bib"));
    } else {
        addMessage(QString(QLatin1String("Don't know format of \"%1\"")).arg(QFileInfo(tempFile.fileName()).fileName()), iconERROR);
        return NULL;
    }

    if (tempFile.open()) {
        bool result = exporter->save(&tempFile, file);
        tempFile.close();
        if (!result)    {
            addMessage(QString(QLatin1String("Could save to temporary file \"%1\"")).arg(QFileInfo(tempFile.fileName()).fileName()), iconERROR);
            delete exporter;
            return QString::null;
        }
    } else {
        addMessage(QString(QLatin1String("Could not open temporary file \"%1\"")).arg(QFileInfo(tempFile.fileName()).fileName()), iconERROR);
        delete exporter;
        return QString::null;
    }

    addMessage(QString(QLatin1String("File \"%1\" was exported to \"%2\"")).arg(QFileInfo(currentTestFile->filename).fileName()).arg(QFileInfo(tempFile.fileName()).fileName()), iconOK);
    return tempFile.fileName();
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
