/***************************************************************************
 *   Copyright (C) 2022 by Thomas Fischer <fischer@unix-ag.uni-kl.de>      *
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

#include <field/FieldLineEdit>
#include <File>
#include <Entry>
#include <models/FileModel>
#include <file/SortFilterFileModel>
#include <preferences/SettingsGlobalKeywordsWidget>

class KBibTeXGUITest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void sortedFilterFileModelSetSourceModel();
    void settingsGlobalKeywordsWidgetAddRemove();

private:
};

void KBibTeXGUITest::initTestCase()
{
    // nothing
}

void KBibTeXGUITest::sortedFilterFileModelSetSourceModel()
{
    File *bibTeXfile = new File();
    // Kirsop, Barbara, and Leslie Chan. (2005) Transforming access to research literature for developing countries. Serials Reviews, 31(4): 246–255.
    QSharedPointer<Entry> entry(new Entry(Entry::etArticle, QStringLiteral("kirsop2005accessrelitdevcountries")));
    bibTeXfile->append(entry);
    entry->insert(Entry::ftTitle, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Transforming access to research literature for developing countries"))));
    entry->insert(Entry::ftAuthor, Value() << QSharedPointer<Person>(new Person(QStringLiteral("Barbara"), QStringLiteral("Kirsop"))) << QSharedPointer<Person>(new Person(QStringLiteral("Leslie"), QStringLiteral("Chan"))));
    entry->insert(Entry::ftYear, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("2005"))));
    entry->insert(Entry::ftJournal, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("Serials Reviews"))));
    entry->insert(Entry::ftVolume, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("31"))));
    entry->insert(Entry::ftNumber, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("4"))));
    entry->insert(Entry::ftPages, Value() << QSharedPointer<PlainText>(new PlainText(QStringLiteral("246--255"))));

    QPointer<FileModel> model = new FileModel();
    model->setBibliographyFile(bibTeXfile);
    QPointer<SortFilterFileModel> sortFilterProxyModel = new SortFilterFileModel();
    sortFilterProxyModel->setSourceModel(model.data());
    QCOMPARE(sortFilterProxyModel->rowCount(), 1);
}

void KBibTeXGUITest::settingsGlobalKeywordsWidgetAddRemove()
{
    QPointer<SettingsGlobalKeywordsWidget> sgkw = new SettingsGlobalKeywordsWidget(nullptr);
    bool gotChanged = false;
    connect(sgkw, &SettingsGlobalKeywordsWidget::changed, this, [&gotChanged] {
        gotChanged = true;
    });

    gotChanged = false;
    sgkw->addKeyword();
    QCOMPARE(gotChanged, true);

    gotChanged = false;
    sgkw->removeKeyword();
    QCOMPARE(gotChanged, true);
}

QTEST_MAIN(KBibTeXGUITest)

#include "kbibtexguitest.moc"
