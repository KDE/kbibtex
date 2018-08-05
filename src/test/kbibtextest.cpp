/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "kbibtextest.h"

#include <QProgressBar>
#include <QTimer>
#include <QLayout>
#include <QMenu>
#include <QIcon>
#include <QPalette>
#include <QDebug>
#include <QPushButton>
#include <QAction>
#include <QListWidget>
#include <QApplication>

#include <KAboutData>
#include <KIconEffect>

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
#include <onlinesearchbiorxiv.h>

int filenameCounter = 0;

static QColor blendColors(const QColor &color1, const QColor &color2, const qreal ratio)
{
    const int r = color1.red() * (1 - ratio) + color2.red() * ratio;
    const int g = color1.green() * (1 - ratio) + color2.green() * ratio;
    const int b = color1.blue() * (1 - ratio) + color2.blue() * ratio;

    return QColor(r, g, b, 255);
}

class TestWidget : public QWidget
{
    Q_OBJECT

private:
    KBibTeXTest *m_parent;
    QPushButton *buttonStartTest;
    QProgressBar *progressBar;
    QAction *actionStartOnlineSearchTests;

public:
    QListWidget *messageList;

    TestWidget(KBibTeXTest *parent)
            : QWidget(parent), m_parent(parent) {
        QGridLayout *layout = new QGridLayout(this);

        buttonStartTest = new QPushButton(QIcon::fromTheme(QStringLiteral("application-x-executable")), QStringLiteral("Start Tests"), this);
        layout->addWidget(buttonStartTest, 0, 0, 1, 1);

        progressBar = new QProgressBar(this);
        layout->addWidget(progressBar, 0, 1, 1, 3);
        progressBar->setVisible(false);

        messageList = new QListWidget(this);
        layout->addWidget(messageList, 1, 0, 4, 4);

        setupMenus();
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
        actionStartOnlineSearchTests = new QAction(QStringLiteral("Online Search"), m_parent);
        connect(actionStartOnlineSearchTests, &QAction::triggered, m_parent, &KBibTeXTest::startOnlineSearchTests);
        menu->addAction(actionStartOnlineSearchTests);
    }

    void setBusy(bool isBusy) {
        buttonStartTest->setEnabled(!isBusy);
        actionStartOnlineSearchTests->setEnabled(!isBusy);
    }
};

KBibTeXTest::KBibTeXTest(QWidget *parent)
        : QDialog(parent), m_running(false), m_isBusy(false)
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
    /// m_onlineSearchList << new OnlineSearchIsbnDB(this); /// disabled as provider switched to a paid model on 2017-12-26
    m_onlineSearchList << new OnlineSearchJStor(this);
    m_onlineSearchList << new OnlineSearchMathSciNet(this);
    m_onlineSearchList << new OnlineSearchMRLookup(this);
    m_onlineSearchList << new OnlineSearchPubMed(this);
    m_onlineSearchList << new OnlineSearchScienceDirect(this);
    m_onlineSearchList << new OnlineSearchSOANASAADS(this);
    m_onlineSearchList << new OnlineSearchSpringerLink(this);
    m_onlineSearchList << new OnlineSearchBioRxiv(this);
    m_currentOnlineSearch = m_onlineSearchList.constBegin();

    setWindowTitle(QStringLiteral("KBibTeX Test Suite"));

    m_testWidget = new TestWidget(this);
    const int fontSize = m_testWidget->fontMetrics().width(QLatin1Char('a'));
    m_testWidget->setMinimumSize(fontSize * 96, fontSize * 48);
    QBoxLayout *boxLayout = new QVBoxLayout(this);
    boxLayout->addWidget(m_testWidget);

    connect(this, &KBibTeXTest::rejected, this, &KBibTeXTest::aboutToQuit);

    addMessage(QString(QStringLiteral("Compiled for %1")).arg(KAboutData::applicationData().version()), statusInfo);
}

void KBibTeXTest::addMessage(const QString &message, const MessageStatus messageStatus)
{
    static const QIcon iconINFO = QIcon::fromTheme(QStringLiteral("dialog-information"));
    static const QIcon iconOK = QIcon::fromTheme(QStringLiteral("dialog-ok-apply"));
    static const QIcon iconERROR = QIcon::fromTheme(QStringLiteral("dialog-cancel"));
    static const QIcon iconAUTH = QIcon::fromTheme(QStringLiteral("dialog-cancel")); // FIXME "dialog-cancel" should be overlay on "dialog-password"
    static const QIcon iconNETWORK = QIcon::fromTheme(QStringLiteral("dialog-cancel")); // FIXME "dialog-cancel" should be overlay on "network-wired"
    QIcon icon;
    switch (messageStatus) {
    case statusInfo: icon = iconINFO; break;
    case statusOk: icon = iconOK; break;
    case statusError: icon = iconERROR; break;
    case statusAuth: icon = iconAUTH; break;
    case statusNetwork: icon = iconNETWORK; break;
    }

    QListWidgetItem *item = icon.isNull() ? new QListWidgetItem(message) : new QListWidgetItem(icon, message);
    item->setToolTip(item->text());
    const QColor originalBgColor = QGuiApplication::palette().color(QPalette::Base);
    switch (messageStatus) {
    case statusInfo: break; ///< nothing to do
    case statusOk: item->setBackgroundColor(blendColors(originalBgColor, Qt::green, .1)); break;
    case statusError: item->setBackgroundColor(blendColors(originalBgColor, Qt::red, .1)); break;
    case statusAuth: item->setBackgroundColor(blendColors(originalBgColor, Qt::yellow, .1)); break;
    case statusNetwork: item->setBackgroundColor(blendColors(originalBgColor, Qt::yellow, .1)); break;
    }

    m_testWidget->messageList->addItem(item);
    m_testWidget->messageList->scrollToBottom();
    qApp->processEvents();
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
    QTimer::singleShot(500, qApp, &QApplication::quit);
}

void KBibTeXTest::startOnlineSearchTests()
{
    m_running = true;
    setBusy(true);
    m_testWidget->messageList->clear();
    qApp->processEvents();
    processNextSearch();
}

void KBibTeXTest::onlineSearchStoppedSearch(int searchResult)
{
    if (searchResult == OnlineSearchAbstract::resultNoError) {
        if (m_currentOnlineSearchNumFoundEntries == 0)
            addMessage(QString(QStringLiteral("Got no error message searching '%1', but found NO entries")).arg((*m_currentOnlineSearch)->label()), statusError);
        else
            addMessage(QString(QStringLiteral("No error searching '%1', found %2 entries")).arg((*m_currentOnlineSearch)->label()).arg(m_currentOnlineSearchNumFoundEntries), statusOk);
    } else if (searchResult == OnlineSearchAbstract::resultAuthorizationRequired) {
        addMessage(QString(QStringLiteral("Authorization required for '%1'")).arg((*m_currentOnlineSearch)->label()), statusAuth);
    } else if (searchResult == OnlineSearchAbstract::resultNetworkError) {
        addMessage(QString(QStringLiteral("Network error for '%1'")).arg((*m_currentOnlineSearch)->label()), statusNetwork);
    } else {
        addMessage(QString(QStringLiteral("Error searching '%1'")).arg((*m_currentOnlineSearch)->label()), statusError);
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
        addMessage(QString(QStringLiteral("Searching '%1'")).arg((*m_currentOnlineSearch)->label()), statusInfo);

        QMap<QString, QString> query;
        query.insert(OnlineSearchAbstract::queryKeyAuthor, QStringLiteral("smith"));
        connect(*m_currentOnlineSearch, &OnlineSearchAbstract::stoppedSearch, this, &KBibTeXTest::onlineSearchStoppedSearch);
        connect(*m_currentOnlineSearch, &OnlineSearchAbstract::foundEntry, this, &KBibTeXTest::onlineSearchFoundEntry);
        connect(*m_currentOnlineSearch, &OnlineSearchAbstract::progress, this, &KBibTeXTest::progress);
        (*m_currentOnlineSearch)->startSearch(query, 3);
    } else {
        addMessage(QStringLiteral("Done testing"), statusInfo);
        setBusy(false);
        m_running = false;
        QTimer::singleShot(500, this, &KBibTeXTest::resetProgress);
    }
}

#include "kbibtextest.moc"
