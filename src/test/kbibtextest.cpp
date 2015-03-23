/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "kbibtextest.h"

#include <QProgressBar>
#include <QTimer>
#include <QLayout>
#include <QMenu>
#include <QIcon>
#include <QDebug>

#include <KApplication>
#include <KPushButton>
#include <KListWidget>
#include <KAction>

#include <onlinesearchacmportal.h>
#include <onlinesearcharxiv.h>
#include <onlinesearchbibsonomy.h>
#include <onlinesearchgooglescholar.h>
#include <onlinesearchcernds.h>
#include <onlinesearchieeexplore.h>
#include <onlinesearchingentaconnect.h>
#include <onlinesearchinspirehep.h>
#include <onlinesearchideasrepec.h>
#include <onlinesearchisbndb.h>
#include <onlinesearchjstor.h>
#include <onlinesearchmathscinet.h>
#include <onlinesearchmrlookup.h>
#include <onlinesearchpubmed.h>
#include <onlinesearchsciencedirect.h>
#include <onlinesearchspringerlink.h>
#include <onlinesearchsoanasaads.h>
#include "version.h"

QIcon iconOK(QLatin1String("dialog-ok-apply"));
QIcon iconERROR(QLatin1String("dialog-cancel"));
QIcon iconINFO(QLatin1String("dialog-information"));
QIcon iconAUTH(QLatin1String("dialog-password"), NULL, QStringList() << QLatin1String("dialog-cancel"));
QIcon iconNETWORK(QLatin1String("network-wired"), NULL, QStringList() << QLatin1String("dialog-cancel"));

int filenameCounter = 0;

class TestWidget : public QWidget
{
private:
    KBibTeXTest *m_parent;
    KPushButton *buttonStartTest;
    QProgressBar *progressBar;
    KAction *actionStartOnlineSearchTests;

public:
    KListWidget *messageList;

    TestWidget(KBibTeXTest *parent)
            : QWidget(parent), m_parent(parent) {
        QGridLayout *layout = new QGridLayout(this);

        buttonStartTest = new KPushButton(QIcon::fromTheme("application-x-executable"), QLatin1String("Start Tests"), this);
        layout->addWidget(buttonStartTest, 0, 0, 1, 1);

        progressBar = new QProgressBar(this);
        layout->addWidget(progressBar, 0, 1, 1, 3);
        progressBar->setVisible(false);

        messageList = new KListWidget(this);
        layout->addWidget(messageList, 1, 0, 4, 4);
    }

    void setProgress(int pos, int total) {
        if (pos < 0 || total < 0) {
            progressBar->setVisible(false);
            progressBar->setMaximum(1);
            progressBar->setValue(0);
        } else {
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
    }

    void setBusy(bool isBusy) {
        buttonStartTest->setEnabled(!isBusy);
        actionStartOnlineSearchTests->setEnabled(!isBusy);
    }
};

KBibTeXTest::KBibTeXTest(QWidget *parent)
        : KDialog(parent), m_running(false), m_isBusy(false)
{
    m_onlineSearchList << new OnlineSearchAcmPortal(this);
    m_onlineSearchList << new OnlineSearchArXiv(this);
    m_onlineSearchList << new OnlineSearchBibsonomy(this);
    m_onlineSearchList << new OnlineSearchCERNDS(this);
    m_onlineSearchList << new OnlineSearchGoogleScholar(this);
    m_onlineSearchList << new OnlineSearchIDEASRePEc(this);
    m_onlineSearchList << new OnlineSearchIEEEXplore(this);
    m_onlineSearchList << new OnlineSearchIngentaConnect(this);
    m_onlineSearchList << new OnlineSearchInspireHep(this);
    m_onlineSearchList << new OnlineSearchIsbnDB(this);
    m_onlineSearchList << new OnlineSearchJStor(this);
    m_onlineSearchList << new OnlineSearchMathSciNet(this);
    m_onlineSearchList << new OnlineSearchMRLookup(this);
    m_onlineSearchList << new OnlineSearchPubMed(this);
    m_onlineSearchList << new OnlineSearchScienceDirect(this);
    m_onlineSearchList << new OnlineSearchSOANASAADS(this);
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

void KBibTeXTest::addMessage(const QString &message, const QIcon &icon)
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

    progress(-1, -1);
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

void KBibTeXTest::resetProgress()
{
    m_testWidget->setProgress(-1, -1);
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
        connect((*m_currentOnlineSearch), SIGNAL(progress(int,int)), this, SLOT(progress(int,int)));
        (*m_currentOnlineSearch)->startSearch(query, 3);
    } else {
        addMessage(QLatin1String("Done testing"), iconINFO);
        setBusy(false);
        m_running = false;
        QTimer::singleShot(500, this, SLOT(resetProgress()));
    }
}
