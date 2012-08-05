#include <QList>

#include <KDialog>

class QProgressBar;
class QListWidget;

class WebSearchAbstract;

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

    QList<WebSearchAbstract *> m_onlineSearchList;
    WebSearchAbstract *m_currentOnlineSearch;
    int m_currentOnlineSearchNumFoundEntries;

    void addMessage(const QString &message, const QString &icon = QString::null);
    void startOnlineSearchTests();
    void processNextSearch();
};
