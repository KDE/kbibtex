#include <QList>

#include <KDialog>

class OnlineSearchAbstract;
class TestWidget;
class File;

class KBibTeXTest : public KDialog
{
    Q_OBJECT

public:
    KBibTeXTest(QWidget *parent = NULL);

private slots:
    void aboutToQuit();
    void startOnlineSearchTests();
    void startTestFileTests();
    void onlineSearchStoppedSearch(int);
    void onlineSearchFoundEntry();

private:
    typedef struct {
        QString filename;
        int numElements, numEntries;
        QString lastEntryId, lastEntryLastAuthorLastName;
        QByteArray md4sum;
    } TestFile;

    bool m_running;
    TestWidget *m_testWidget;

    QList<OnlineSearchAbstract *> m_onlineSearchList;
    OnlineSearchAbstract *m_currentOnlineSearch;
    int m_currentOnlineSearchNumFoundEntries;

    void addMessage(const QString &message, const KIcon &icon = KIcon());

    void processNextSearch();

    File *loadFile(const QString &absoluteFilename, TestFile *currentTestFile);
    QString saveFile(File *file, TestFile *currentTestFile);
    TestFile *createTestFile(const QString &filename, int numElements, int numEntries, const QString &lastEntryId, const QString &lastEntryLastAuthorLastName, const QString &md4sumHex);
};
