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

#include <QtTest>

#include "entry.h"

class KBibTeXDataTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    /**
     * This test is not designed to fail under regular circumstances.
     * Instead, this test should be run with 'valgrind' to observe any
     * irregularities in memory management or access.
     */
    void createAndRemoveValueFromEntries();

private:
};


void KBibTeXDataTest::createAndRemoveValueFromEntries()
{
    static const QString keyData[] = {Entry::ftAuthor, Entry::ftISBN};

    Entry entry;
    for (const QString &key : keyData) {
        for (int i = 0; i < 2; ++i) {
            entry.remove(key);
            Value v;
            entry.insert(key, v);
        }
    }
}

void KBibTeXDataTest::initTestCase()
{
    // TODO
}

QTEST_MAIN(KBibTeXDataTest)

#include "kbibtexdatatest.moc"
