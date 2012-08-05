#include <KApplication>
#include <KIcon>

#include <QProgressBar>
#include <QListWidget>
#include <QTimer>

#include <websearchacmportal.h>
#include <websearcharxiv.h>
#include <websearchbibsonomy.h>
#include <websearchgooglescholar.h>
#include <websearchieeexplore.h>
#include <websearchjstor.h>
#include <websearchpubmed.h>
#include <websearchsciencedirect.h>
#include <websearchspringerlink.h>
#include "kbibtextest.h"

const QString iconOK = QLatin1String("dialog-ok-apply");
const QString iconERROR = QLatin1String("dialog-cancel");
const QString iconINFO = QLatin1String("dialog-information");

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

void KBibTeXTest::addMessage(const QString &message, const QString &icon)
{
    QListWidgetItem *item = icon.isEmpty() ? new QListWidgetItem(message) : new QListWidgetItem(KIcon(icon), message);
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
    m_onlineSearchList << new WebSearchAcmPortal(this);
    m_onlineSearchList << new WebSearchArXiv(this);
    m_onlineSearchList << new WebSearchBibsonomy(this);
    m_onlineSearchList << new WebSearchGoogleScholar(this);
    m_onlineSearchList << new WebSearchIEEEXplore(this);
    m_onlineSearchList << new WebSearchJStor(this);
    m_onlineSearchList << new WebSearchPubMed(this);
    m_onlineSearchList << new WebSearchScienceDirect(this);
    m_onlineSearchList << new WebSearchSpringerLink(this);

    processNextSearch();
}

void KBibTeXTest::onlineSearchStoppedSearch(int searchResult)
{
    if (searchResult == WebSearchAbstract::resultNoError) {
        if (m_currentOnlineSearchNumFoundEntries==0)
            addMessage(QString(QLatin1String("No error searching \"%1\", but found no entries")).arg(m_currentOnlineSearch->label()), iconERROR);
            else
        addMessage(QString(QLatin1String("No error searching \"%1\", found %2 entries")).arg(m_currentOnlineSearch->label()).arg(m_currentOnlineSearchNumFoundEntries), iconOK);
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
        query.insert(WebSearchAbstract::queryKeyAuthor, QLatin1String("smith"));
        connect(m_currentOnlineSearch, SIGNAL(stoppedSearch(int)), this, SLOT(onlineSearchStoppedSearch(int)));
        connect(m_currentOnlineSearch, SIGNAL(foundEntry(Entry*)), this, SLOT(onlineSearchFoundEntry()));
        m_currentOnlineSearch->startSearch(query, 3);
    } else
        addMessage(QLatin1String("Done testing"), iconINFO);
}
