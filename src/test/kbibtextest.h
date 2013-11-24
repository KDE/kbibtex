#ifndef KBIBTEXTEST_H
#define KBIBTEXTEST_H

#include <QList>

#include <KDialog>

class OnlineSearchAbstract;
class TestWidget;
class File;

class KBibTeXTest : public KDialog
{
    Q_OBJECT

public:
    typedef struct {
        QString filename;
        int numElements, numEntries;
        QString lastEntryId, lastEntryLastAuthorLastName;
        QByteArray hashAuthors, hashFilesUrlsDoi;
    } TestFile;

    explicit KBibTeXTest(QWidget *parent = NULL);

    QList<TestFile *> testFiles;

private slots:
    void aboutToQuit();
    void startOnlineSearchTests();
    void startAllTestFileTests();
    void startTestFileTest(int);
    void onlineSearchStoppedSearch(int);
    void onlineSearchFoundEntry();
    void progress(int, int);
    void resetProgress();

private:
    bool m_running;
    TestWidget *m_testWidget;
    bool m_isBusy;

    QList<OnlineSearchAbstract *> m_onlineSearchList;
    QList<OnlineSearchAbstract *>::ConstIterator m_currentOnlineSearch;
    int m_currentOnlineSearchNumFoundEntries;

    void addMessage(const QString &message, const KIcon &icon = KIcon());
    void setBusy(bool isBusy);

    void processNextSearch();
    void processFileTest(TestFile *testFile);

    File *loadFile(const QString &absoluteFilename, TestFile *currentTestFile);
    QString saveFile(File *file, TestFile *currentTestFile);
    TestFile *createTestFile(const QString &filename, int numElements, int numEntries, const QString &lastEntryId, const QString &lastEntryLastAuthorLastName, const QString &hashAuthors, const QString &hashFilesUrlsDoi);
};

#endif // KBIBTEXTEST_H
