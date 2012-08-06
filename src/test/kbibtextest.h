#include <QList>

#include <KDialog>

class QProgressBar;
class QListWidget;

class OnlineSearchAbstract;

class KBibTeXTest : public KDialog
{
    Q_OBJECT

public:
    KBibTeXTest(QWidget *parent = NULL);

private slots:
    void aboutToQuit();
    void startTests();
    void onlineSearchStoppedSearch(int);
    void onlineSearchFoundEntry();

private:
    bool m_running;
    QListWidget *m_messageList;

    QList<OnlineSearchAbstract *> m_onlineSearchList;
    OnlineSearchAbstract *m_currentOnlineSearch;
    int m_currentOnlineSearchNumFoundEntries;

    void addMessage(const QString &message, const KIcon &icon = KIcon());
    void startOnlineSearchTests();
    void processNextSearch();
};
