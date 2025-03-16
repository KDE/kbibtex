/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2025 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <onlinesearch/OnlineSearchGoogleBooks>
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
#include <onlinesearch/OnlineSearchUnpaywall>
#include <onlinesearch/OnlineSearchZbMath>

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

    void setupMenus(QList<OnlineSearchAbstract*> &onlineSearchList)
    {
        QMenu *menu = new QMenu(buttonStartTest);
        buttonStartTest->setMenu(menu);

        QMenu *onlineSearchMenu = new QMenu(QStringLiteral("Online Search"), menu);
        menu->addMenu(onlineSearchMenu);
        actionStartOnlineSearchTests = new QAction(QStringLiteral("All engines"), m_parent);
        connect(actionStartOnlineSearchTests, &QAction::triggered, m_parent, [this]() {
            m_parent->onlineSearchActiveList = QList(m_parent->onlineSearchList);
            m_parent->currentOnlineSearch = m_parent->onlineSearchActiveList.constBegin();
            m_parent->startOnlineSearchTests();
        });
        onlineSearchMenu->addAction(actionStartOnlineSearchTests);

        onlineSearchMenu->addSeparator();

        for (const auto onlineSearch : onlineSearchList) {
            QAction *searchAction = new QAction(onlineSearch->label(), m_parent);
            connect(searchAction, &QAction::triggered, m_parent, [this, onlineSearch]() {
                m_parent->onlineSearchActiveList = {onlineSearch};
                m_parent->currentOnlineSearch = m_parent->onlineSearchActiveList.constBegin();
                m_parent->startOnlineSearchTests();
            });
            onlineSearchMenu->addAction(searchAction);
        }
    }

    void setBusy(bool isBusy) {
        buttonStartTest->setEnabled(!isBusy);
    }
};


KBibTeXTest::KBibTeXTest(QWidget *parent)
        : QDialog(parent), m_running(false), m_isBusy(false)
{
    onlineSearchList << new OnlineSearchAcmPortal(this);
    onlineSearchList << new OnlineSearchArXiv(this);
    onlineSearchList << new OnlineSearchBibsonomy(this);
    onlineSearchList << new OnlineSearchCERNDS(this);
    onlineSearchList << new OnlineSearchGoogleBooks(this);
    onlineSearchList << new OnlineSearchGoogleScholar(this);
    onlineSearchList << new OnlineSearchIDEASRePEc(this);
    onlineSearchList << new OnlineSearchIEEEXplore(this);
    onlineSearchList << new OnlineSearchIngentaConnect(this);
    onlineSearchList << new OnlineSearchInspireHep(this);
#ifdef HAVE_WEBENGINEWIDGETS
    onlineSearchList << new OnlineSearchJStor(this);
#endif // HAVE_WEBENGINEWIDGETS
    onlineSearchList << new OnlineSearchMathSciNet(this);
    onlineSearchList << new OnlineSearchMRLookup(this);
    onlineSearchList << new OnlineSearchPubMed(this);
    onlineSearchList << new OnlineSearchScienceDirect(this);
    onlineSearchList << new OnlineSearchSOANASAADS(this);
    onlineSearchList << new OnlineSearchSpringerLink(this);
    onlineSearchList << new OnlineSearchBioRxiv(OnlineSearchBioRxiv::Rxiv::bioRxiv, this);
    onlineSearchList << new OnlineSearchBioRxiv(OnlineSearchBioRxiv::Rxiv::medRxiv, this);
    onlineSearchList << new OnlineSearchSemanticScholar(this);
    onlineSearchList << new OnlineSearchUnpaywall(this);
    onlineSearchList << new OnlineSearchZbMath(this);
    onlineSearchActiveList = QList(onlineSearchList);
    currentOnlineSearch = onlineSearchActiveList.constBegin();

    setWindowTitle(QStringLiteral("KBibTeX Test Suite"));

    m_testWidget = new TestWidget(this);
    m_testWidget->setupMenus(onlineSearchList);

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
    if (currentOnlineSearch != onlineSearchActiveList.constEnd())
        disconnect(*currentOnlineSearch, &OnlineSearchAbstract::stoppedSearch, this, &KBibTeXTest::onlineSearchStoppedSearch);
    if (searchResult == OnlineSearchAbstract::resultNoError) {
        if (m_currentOnlineSearchNumFoundEntries == 0)
            addMessage(QString(QStringLiteral("Got no error message searching '%1', but found NO entries")).arg((*currentOnlineSearch)->label()), MessageStatus::Error);
        else
            addMessage(QString(QStringLiteral("No error searching '%1', found %2 entries")).arg((*currentOnlineSearch)->label()).arg(m_currentOnlineSearchNumFoundEntries), MessageStatus::Ok);
    } else if (searchResult == OnlineSearchAbstract::resultAuthorizationRequired) {
        addMessage(QString(QStringLiteral("Authorization required for '%1'")).arg((*currentOnlineSearch)->label()), MessageStatus::Auth);
    } else if (searchResult == OnlineSearchAbstract::resultNetworkError) {
        addMessage(QString(QStringLiteral("Network error for '%1'")).arg((*currentOnlineSearch)->label()), MessageStatus::Network);
    } else {
        addMessage(QString(QStringLiteral("Error searching '%1'")).arg((*currentOnlineSearch)->label()), MessageStatus::Error);
    }
    currentOnlineSearch++;

    progress(-1, -1);
    processNextSearch();
}

void KBibTeXTest::progress(int pos, int total)
{
    m_testWidget->setProgress(pos, total);
}

void KBibTeXTest::processNextSearch()
{
    if (m_running && currentOnlineSearch != onlineSearchActiveList.constEnd()) {
        setBusy(true);
        m_currentOnlineSearchNumFoundEntries = 0;
        addMessage(QString(QStringLiteral("Searching '%1'")).arg((*currentOnlineSearch)->label()), MessageStatus::Info);

        QMap<OnlineSearchAbstract::QueryKey, QString> query;
        OnlineSearchBioRxiv* onlineSearchBioRxiv{nullptr};
        if (qobject_cast<OnlineSearchSemanticScholar*>(*currentOnlineSearch) != nullptr)
            /// Semantic Scholar cannot search for last names, but for DOIs or arXiv IDs instead
            query.insert(OnlineSearchAbstract::QueryKey::FreeText, QStringLiteral("10.1002/smj.863"));
        else if ((onlineSearchBioRxiv = qobject_cast<OnlineSearchBioRxiv*>(*currentOnlineSearch)) != nullptr)
            /// BioRxiv/MedRxiv cannot search for last names, but for DOIs instead
            query.insert(OnlineSearchAbstract::QueryKey::FreeText, onlineSearchBioRxiv->rxiv() == OnlineSearchBioRxiv::Rxiv::medRxiv ? QStringLiteral("10.1101/2025.03.14.25323943") : QStringLiteral("10.1101/2025.03.14.643318"));
        else if (qobject_cast<OnlineSearchSpringerLink*>(*currentOnlineSearch) != nullptr)
            /// Searching for author is a Premium Plan feature at Springer Nature, so search for DOI instead
            query.insert(OnlineSearchAbstract::QueryKey::FreeText, QStringLiteral("10.1007/s42864-024-00293-x"));
        else if (qobject_cast<OnlineSearchUnpaywall*>(*currentOnlineSearch) != nullptr)
            /// Unpaywall cannot search for last names, but for DOIs of open access publications
            query.insert(OnlineSearchAbstract::QueryKey::FreeText, QStringLiteral("10.1002/andp.201600209"));
        else if (qobject_cast<OnlineSearchGoogleBooks*>(*currentOnlineSearch) != nullptr)
            /// Google Books can only search for ISBN
            query.insert(OnlineSearchAbstract::QueryKey::FreeText, QStringLiteral("1493905244"));
        else {
            static const QStringList lastNames{QStringLiteral("Smith"), QStringLiteral("Jones"), QStringLiteral("Andersson"), QStringLiteral("Ivanova"), QStringLiteral("Wang"), QStringLiteral("Gonzalez"), QStringLiteral("Garcia"), QStringLiteral("Lopez"), QStringLiteral("Ahmed"), QStringLiteral("Nkosi"), QStringLiteral("Kim"), QStringLiteral("Chen"), QStringLiteral("Devi"), QStringLiteral("Khan"), QStringLiteral("Johansson"), QStringLiteral("Sharipov"), QStringLiteral("Korhonen"), QStringLiteral("Muller"), QStringLiteral("Murphy"), QStringLiteral("Papadopoulos"), QStringLiteral("Rossi"), QStringLiteral("Hernandez"), QStringLiteral("Williams"), QStringLiteral("Zhang"), QStringLiteral("Singh"), QStringLiteral("Kumar")};
            query.insert(OnlineSearchAbstract::QueryKey::Author, lastNames[QRandomGenerator::global()->bounded(lastNames.count())]);
        }
        connect(*currentOnlineSearch, &OnlineSearchAbstract::stoppedSearch, this, &KBibTeXTest::onlineSearchStoppedSearch);
        connect(*currentOnlineSearch, &OnlineSearchAbstract::foundEntry, this, [this]() {
            ++m_currentOnlineSearchNumFoundEntries;
        });
        connect(*currentOnlineSearch, &OnlineSearchAbstract::progress, this, &KBibTeXTest::progress);
        (*currentOnlineSearch)->startSearch(query, 3);
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
