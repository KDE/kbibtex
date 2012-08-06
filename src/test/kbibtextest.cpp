#include <KApplication>
#include <KIcon>

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

KIcon iconOK(QLatin1String("dialog-ok-apply"));
KIcon iconERROR(QLatin1String("dialog-cancel"));
KIcon iconINFO(QLatin1String("dialog-information"));
KIcon iconAUTH(QLatin1String("dialog-password"), NULL, QStringList() << QLatin1String("dialog-cancel"));
KIcon iconNETWORK(QLatin1String("network-wired"), NULL, QStringList() << QLatin1String("dialog-cancel"));

KBibTeXTest::KBibTeXTest(QWidget *parent)
        : KDialog(parent), m_running(false)
{
    setPlainCaption(QLatin1String("KBibTeX Test Suite"));
    setButtons(KDialog::Close);

    m_messageList = new QListWidget(this);
    const int fontSize = m_messageList->fontMetrics().width(QLatin1Char('a'));
    m_messageList->setMinimumSize(fontSize * 96, fontSize * 48);
    setMainWidget(m_messageList);

    connect(this, SIGNAL(closeClicked()), this, SLOT(aboutToQuit()));

    m_running = true;
    QTimer::singleShot(500, this, SLOT(startTests()));
}

void KBibTeXTest::addMessage(const QString &message, const KIcon &icon)
{
    QListWidgetItem *item = icon.isNull() ? new QListWidgetItem(message) : new QListWidgetItem(icon, message);
    m_messageList->addItem(item);
    m_messageList->scrollToBottom();
}

void KBibTeXTest::aboutToQuit()
{
    m_running = false;
    QTimer::singleShot(500, kapp, SLOT(quit()));
}

void KBibTeXTest::startTests()
{
    startOnlineSearchTests();
}

void KBibTeXTest::startOnlineSearchTests()
{
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
    } else
        addMessage(QLatin1String("Done testing"), iconINFO);
}
