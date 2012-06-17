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
    void startTests();
    void onlineSearchStoppedSearch(int);

private:
    QListWidget *m_messageList;

    QList<OnlineSearchAbstract *> m_onlineSearchList;
    OnlineSearchAbstract *m_currentOnlineSearch;

    void addMessage(const QString &message, const QString &icon = QString::null);
    void startOnlineSearchTests();
};
