/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QPushButton>
#include <QAction>
#include <QListWidget>
#include <QApplication>
#include <QRandomGenerator>

#include <KAboutData>

#include <onlinesearch/OnlineSearchAcmPortal>
#include <onlinesearch/OnlineSearchArXiv>
#include <onlinesearch/OnlineSearchBibsonomy>
#include <onlinesearch/OnlineSearchGoogleScholar>
#include <onlinesearch/OnlineSearchCERNDS>
#include <onlinesearch/OnlineSearchIEEEXplore>
#include <onlinesearch/OnlineSearchIngentaConnect>
#include <onlinesearch/OnlineSearchInspireHep>
#include <onlinesearch/OnlineSearchIDEASRePEc>
#ifdef HAVE_WEBENGINEWIDGETS
#include <onlinesearch/OnlineSearchJStor>
#endif // HAVE_WEBENGINEWIDGETS
#include <onlinesearch/OnlineSearchMathSciNet>
#include <onlinesearch/OnlineSearchMRLookup>
#include <onlinesearch/OnlineSearchPubMed>
#include <onlinesearch/OnlineSearchScienceDirect>
#include <onlinesearch/OnlineSearchSpringerLink>
#include <onlinesearch/OnlineSearchSOANASAADS>
#include <onlinesearch/OnlineSearchBioRxiv>
#include <onlinesearch/OnlineSearchSemanticScholar>

static QColor blendColors(const QColor &color1, const QColor &color2, const qreal ratio)
{
    const int r = static_cast<int>(color1.red() * (1 - ratio) + color2.red() * ratio);
    const int g = static_cast<int>(color1.green() * (1 - ratio) + color2.green() * ratio);
    const int b = static_cast<int>(color1.blue() * (1 - ratio) + color2.blue() * ratio);

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
#ifdef HAVE_WEBENGINEWIDGETS
    m_onlineSearchList << new OnlineSearchJStor(this);
#endif // HAVE_WEBENGINEWIDGETS
    m_onlineSearchList << new OnlineSearchMathSciNet(this);
    m_onlineSearchList << new OnlineSearchMRLookup(this);
    m_onlineSearchList << new OnlineSearchPubMed(this);
    m_onlineSearchList << new OnlineSearchScienceDirect(this);
    m_onlineSearchList << new OnlineSearchSOANASAADS(this);
    m_onlineSearchList << new OnlineSearchSpringerLink(this);
    m_onlineSearchList << new OnlineSearchBioRxiv(this);
    m_onlineSearchList << new OnlineSearchSemanticScholar(this);
    m_currentOnlineSearch = m_onlineSearchList.constBegin();

    setWindowTitle(QStringLiteral("KBibTeX Test Suite"));

    m_testWidget = new TestWidget(this);
#if QT_VERSION >= 0x050b00
    const int fontSize = m_testWidget->fontMetrics().horizontalAdvance(QLatin1Char('a'));
#else // QT_VERSION >= 0x050b00
    const int fontSize = m_testWidget->fontMetrics().width(QLatin1Char('a'));
#endif // QT_VERSION >= 0x050b00
    m_testWidget->setMinimumSize(fontSize * 96, fontSize * 48);
    QBoxLayout *boxLayout = new QVBoxLayout(this);
    boxLayout->addWidget(m_testWidget);

    connect(this, &KBibTeXTest::rejected, this, &KBibTeXTest::aboutToQuit);

    addMessage(QString(QStringLiteral("Compiled for %1")).arg(KAboutData::applicationData().version()), MessageStatus::Info);
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
    case MessageStatus::Info: icon = iconINFO; break;
    case MessageStatus::Ok: icon = iconOK; break;
    case MessageStatus::Error: icon = iconERROR; break;
    case MessageStatus::Auth: icon = iconAUTH; break;
    case MessageStatus::Network: icon = iconNETWORK; break;
    }

    QListWidgetItem *item = icon.isNull() ? new QListWidgetItem(message) : new QListWidgetItem(icon, message);
    item->setToolTip(item->text());
    const QColor originalBgColor = QGuiApplication::palette().color(QPalette::Base);
    switch (messageStatus) {
    case MessageStatus::Info: break; ///< nothing to do
    case MessageStatus::Ok: item->setBackground(QBrush(blendColors(originalBgColor, Qt::green, .1))); break;
    case MessageStatus::Error: item->setBackground(QBrush(blendColors(originalBgColor, Qt::red, .1))); break;
    case MessageStatus::Auth: item->setBackground(QBrush(blendColors(originalBgColor, Qt::yellow, .1))); break;
    case MessageStatus::Network: item->setBackground(QBrush(blendColors(originalBgColor, Qt::yellow, .1))); break;
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
    if (m_currentOnlineSearch != m_onlineSearchList.constEnd())
        disconnect(*m_currentOnlineSearch, &OnlineSearchAbstract::stoppedSearch, this, &KBibTeXTest::onlineSearchStoppedSearch);
    if (searchResult == OnlineSearchAbstract::resultNoError) {
        if (m_currentOnlineSearchNumFoundEntries == 0)
            addMessage(QString(QStringLiteral("Got no error message searching '%1', but found NO entries")).arg((*m_currentOnlineSearch)->label()), MessageStatus::Error);
        else
            addMessage(QString(QStringLiteral("No error searching '%1', found %2 entries")).arg((*m_currentOnlineSearch)->label()).arg(m_currentOnlineSearchNumFoundEntries), MessageStatus::Ok);
    } else if (searchResult == OnlineSearchAbstract::resultAuthorizationRequired) {
        addMessage(QString(QStringLiteral("Authorization required for '%1'")).arg((*m_currentOnlineSearch)->label()), MessageStatus::Auth);
    } else if (searchResult == OnlineSearchAbstract::resultNetworkError) {
        addMessage(QString(QStringLiteral("Network error for '%1'")).arg((*m_currentOnlineSearch)->label()), MessageStatus::Network);
    } else {
        addMessage(QString(QStringLiteral("Error searching '%1'")).arg((*m_currentOnlineSearch)->label()), MessageStatus::Error);
    }
    m_currentOnlineSearch++;

    progress(-1, -1);
    processNextSearch();
}

void KBibTeXTest::progress(int pos, int total)
{
    m_testWidget->setProgress(pos, total);
}

void KBibTeXTest::processNextSearch()
{
    if (m_running && m_currentOnlineSearch != m_onlineSearchList.constEnd()) {
        setBusy(true);
        m_currentOnlineSearchNumFoundEntries = 0;
        addMessage(QString(QStringLiteral("Searching '%1'")).arg((*m_currentOnlineSearch)->label()), MessageStatus::Info);

        QMap<OnlineSearchAbstract::QueryKey, QString> query;
        if (qobject_cast<OnlineSearchSemanticScholar *>(*m_currentOnlineSearch) != nullptr)
            /// Semantic Scholar cannot search for last names, but for DOIs or arXiv IDs instead
            query.insert(OnlineSearchAbstract::QueryKey::FreeText, QStringLiteral("10.1002/smj.863"));
        else {
            static const QStringList lastNames{QStringLiteral("Smith"), QStringLiteral("Jones"), QStringLiteral("Andersson"), QStringLiteral("Ivanova"), QStringLiteral("Wang"), QStringLiteral("Gonzalez"), QStringLiteral("Garcia"), QStringLiteral("Lopez"), QStringLiteral("Ahmed"), QStringLiteral("Nkosi"), QStringLiteral("Kim"), QStringLiteral("Chen"), QStringLiteral("Devi"), QStringLiteral("Khan"), QStringLiteral("Johansson"), QStringLiteral("Sharipov"), QStringLiteral("Korhonen"), QStringLiteral("Muller"), QStringLiteral("Murphy"), QStringLiteral("Papadopoulos"), QStringLiteral("Rossi"), QStringLiteral("Hernandez"), QStringLiteral("Williams"), QStringLiteral("Zhang"), QStringLiteral("Singh"), QStringLiteral("Kumar")};
            query.insert(OnlineSearchAbstract::QueryKey::Author, lastNames[QRandomGenerator::global()->bounded(lastNames.count())]);
        }
        connect(*m_currentOnlineSearch, &OnlineSearchAbstract::stoppedSearch, this, &KBibTeXTest::onlineSearchStoppedSearch);
        connect(*m_currentOnlineSearch, &OnlineSearchAbstract::foundEntry, this, [this]() {
            ++m_currentOnlineSearchNumFoundEntries;
        });
        connect(*m_currentOnlineSearch, &OnlineSearchAbstract::progress, this, &KBibTeXTest::progress);
        (*m_currentOnlineSearch)->startSearch(query, 3);
    } else {
        addMessage(QStringLiteral("Done testing"), MessageStatus::Info);
        setBusy(false);
        m_running = false;
        QTimer::singleShot(500, this, [this]() {
            m_testWidget->setProgress(-1, -1);
        });
    }
}

#include "kbibtextest.moc"
