#include <KApplication>
#include <KIcon>

#include <QLayout>
#include <QProgressBar>
#include <QListWidget>
#include <QTimer>

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
#include "kbibtextest.h"

const QString iconOK = QLatin1String("dialog-ok-apply");
const QString iconERROR = QLatin1String("dialog-cancel");
const QString iconINFO = QLatin1String("dialog-information");

KBibTeXTest::KBibTeXTest(QWidget *parent)
    : KDialog(parent)
{
    setPlainCaption(QLatin1String("KBibTeX Test Suite"));
    setButtons(KDialog::Close);

    QLayout *layout = new QVBoxLayout(this);
    m_messageList = new QListWidget(this);
    layout->addWidget(m_messageList);
    m_messageList->setMinimumSize(768, 512);
    setMainWidget(m_messageList);

    kapp->quitOnLastWindowClosed();

    QTimer::singleShot(500, this, SLOT(startTests()));
}

void KBibTeXTest::addMessage(const QString &message, const QString &icon)
{
    QListWidgetItem *item = icon.isEmpty() ? new QListWidgetItem(message) : new QListWidgetItem(KIcon(icon), message);
    m_messageList->addItem(item);
    m_messageList->scrollToBottom();
}

void KBibTeXTest::startTests()
{
    startOnlineSearchTests();
}

void KBibTeXTest::startOnlineSearchTests()
{
    m_onlineSearchList << new OnlineSearchAcmPortal(this) << new OnlineSearchArXiv(this) << new OnlineSearchBibsonomy(this) << new OnlineSearchGoogleScholar(this) << new OnlineSearchIEEEXplore(this) << new OnlineSearchIngentaConnect(this) << new OnlineSearchJStor(this) << new OnlineSearchPubMed(this) << new OnlineSearchScienceDirect(this) << new OnlineSearchSpringerLink(this);

    if (!m_onlineSearchList.isEmpty()) {
        m_currentOnlineSearch = m_onlineSearchList.first();
        m_onlineSearchList.removeFirst();
        addMessage(QString(QLatin1String("Searching \"%1\"")).arg(m_currentOnlineSearch->label()), iconINFO);

        QMap<QString, QString> query;
        query.insert(OnlineSearchAbstract::queryKeyAuthor, QLatin1String("smith"));
        connect(m_currentOnlineSearch, SIGNAL(stoppedSearch(int)), this, SLOT(onlineSearchStoppedSearch(int)));
        m_currentOnlineSearch->startSearch(query, 3);
    } else
        addMessage(QLatin1String("Done testing"), iconINFO);
}

void KBibTeXTest::onlineSearchStoppedSearch(int searchResult)
{
    if (searchResult == OnlineSearchAbstract::resultNoError) {
        addMessage(QString(QLatin1String("No error searching \"%1\"")).arg(m_currentOnlineSearch->label()), iconOK);
    } else {
        addMessage(QString(QLatin1String("Error searching \"%1\"")).arg(m_currentOnlineSearch->label()), iconERROR);
    }

    if (!m_onlineSearchList.isEmpty()) {
        m_currentOnlineSearch = m_onlineSearchList.first();
        m_onlineSearchList.removeFirst();
        addMessage(QString(QLatin1String("Searching \"%1\"")).arg(m_currentOnlineSearch->label()), iconINFO);

        QMap<QString, QString> query;
        query.insert(OnlineSearchAbstract::queryKeyAuthor, QLatin1String("smith"));
        connect(m_currentOnlineSearch, SIGNAL(stoppedSearch(int)), this, SLOT(onlineSearchStoppedSearch(int)));
        m_currentOnlineSearch->startSearch(query, 3);
    } else
        addMessage(QLatin1String("Done testing"), iconINFO);
}
