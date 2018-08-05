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

#ifndef KBIBTEXTEST_H
#define KBIBTEXTEST_H

#include <QList>
#include <QDialog>
#include <QIcon>

class OnlineSearchAbstract;
class TestWidget;

class KBibTeXTest : public QDialog
{
    Q_OBJECT

public:
    explicit KBibTeXTest(QWidget *parent = nullptr);

public slots:
    void startOnlineSearchTests();

private slots:
    void aboutToQuit();
    void onlineSearchStoppedSearch(int);
    void onlineSearchFoundEntry();
    void progress(int, int);
    void resetProgress();

private:
    enum MessageStatus { statusInfo, statusOk, statusError, statusAuth, statusNetwork };

    bool m_running;
    TestWidget *m_testWidget;
    bool m_isBusy;

    QList<OnlineSearchAbstract *> m_onlineSearchList;
    QList<OnlineSearchAbstract *>::ConstIterator m_currentOnlineSearch;
    int m_currentOnlineSearchNumFoundEntries;

    void addMessage(const QString &message, const MessageStatus messageStatus);
    void setBusy(bool isBusy);

    void processNextSearch();
};

#endif // KBIBTEXTEST_H
