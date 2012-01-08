/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QLayout>
#include <QTreeView>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KPushButton>
#include <KLocale>

#include "fileimporterbibtex.h"
#include "idsuggestions.h"
#include "settingsidsuggestionswidget.h"

const int FormatStringRole = Qt::UserRole + 7811;

class IdSuggestionsModel : public QAbstractListModel
{
private:
    QStringList m_formatStringList;
    IdSuggestions *m_idSuggestions;
    static const QString exampleBibTeXEntryString;
    static QSharedPointer<const Entry> exampleBibTeXEntry;

public:
    IdSuggestionsModel(QObject *parent = NULL)
            : QAbstractListModel(parent) {
        m_idSuggestions = new IdSuggestions();

        if (exampleBibTeXEntry.isNull()) {
            static FileImporterBibTeX fileImporterBibTeX;
            File *file = fileImporterBibTeX.fromString(exampleBibTeXEntryString);
            exampleBibTeXEntry = file->first().dynamicCast<const Entry>();
            delete file;
        }
    }

    ~IdSuggestionsModel() {
        delete m_idSuggestions;
    }

    void setFormatStringList(const QStringList &formatStringList) {
        m_formatStringList = formatStringList;
        reset();
    }

    QStringList formatStringList() const {
        return m_formatStringList;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        if (parent == QModelIndex())
            return m_formatStringList.count();
        else
            return 0;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        if (index.row() < 0 || index.row() >= m_formatStringList.count())
            return QVariant();

        switch (role) {
        case Qt::DecorationRole:
            return KIcon("view-filter");
        case Qt::ToolTipRole:
            return i18n("<qt>Structure:<ul><li>%1</li></ul>Example: %2</qt>", m_idSuggestions->formatStrToHuman(m_formatStringList[index.row()]).join(QLatin1String("</li><li>")), m_idSuggestions->formatId(*exampleBibTeXEntry, m_formatStringList[index.row()]));
        case Qt::DisplayRole:
            return m_idSuggestions->formatId(*exampleBibTeXEntry, m_formatStringList[index.row()]);
        case FormatStringRole:
            return m_formatStringList[index.row()];
        default:
            return QVariant();
        }

        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const {
        if (role == Qt::DisplayRole && section == 0)
            return i18n("Id Suggestions");

        return QVariant();
    }

    bool moveUp(const QModelIndex &index) {
        int row = index.row();
        if (row < 1 || row >= m_formatStringList.count())
            return false;

        const QString formatString = m_formatStringList[row];
        m_formatStringList.removeAt(row);
        m_formatStringList.insert(row - 1, formatString);

        reset();
        return true;
    }

    bool moveDown(const QModelIndex &index) {
        int row = index.row();
        if (row < 0 || row >= m_formatStringList.count() - 1)
            return false;

        const QString formatString = m_formatStringList[row];
        m_formatStringList.removeAt(row);
        m_formatStringList.insert(row + 1, formatString);

        reset();
        return true;
    }
};

const QString IdSuggestionsModel::exampleBibTeXEntryString = QLatin1String("@Article{ dijkstra1983terminationdetect,\nauthor = {Edsger W. Dijkstra and W. H. J. Feijen and A. J. M. {van Gasteren}},\ntitle = {{Derivation of a Termination Detection Algorithm for Distributed Computations}},\njournal = {Information Processing Letters},\nvolume = 16,\nnumber = 5,\npages = {217--219},\nmonth = jun,\nyear = 1983\n}");
QSharedPointer<const Entry> IdSuggestionsModel::exampleBibTeXEntry;

class SettingsIdSuggestionsWidget::SettingsIdSuggestionsWidgetPrivate
{
private:
    SettingsIdSuggestionsWidget *p;

    KSharedConfigPtr config;
    KConfigGroup configGroup;

public:
    QTreeView *treeViewSuggestions;
    IdSuggestionsModel *idSuggestionsModel;
    KPushButton *buttonNewSuggestion, *buttonEditSuggestion, *buttonDeleteSuggestion, *buttonSuggestionUp, *buttonSuggestionDown;

    SettingsIdSuggestionsWidgetPrivate(SettingsIdSuggestionsWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroup(config, IdSuggestions::configGroupName) {
        // nothing
    }

    void loadState() {
        idSuggestionsModel->setFormatStringList(configGroup.readEntry(IdSuggestions::keyFormatStringList, IdSuggestions::defaultFormatStringList));
    }

    void saveState() {
        configGroup.writeEntry(IdSuggestions::keyFormatStringList, idSuggestionsModel->formatStringList());
        config->sync();
    }

    void resetToDefaults() {
        idSuggestionsModel->setFormatStringList(IdSuggestions::defaultFormatStringList);
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        treeViewSuggestions = new QTreeView(p);
        layout->addWidget(treeViewSuggestions, 0, 0, 7, 1);
        idSuggestionsModel = new IdSuggestionsModel(treeViewSuggestions);
        treeViewSuggestions->setModel(idSuggestionsModel);
        treeViewSuggestions->setRootIsDecorated(false);
        connect(treeViewSuggestions->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), p, SLOT(itemChanged(QModelIndex)));

        buttonNewSuggestion = new KPushButton(KIcon("list-add"), i18n("Add..."), p);
        layout->addWidget(buttonNewSuggestion, 0, 1, 1, 1);
        // TODO no functionality yet, disable button
        buttonNewSuggestion->setEnabled(false);

        buttonEditSuggestion = new KPushButton(KIcon("document-edit"), i18n("Edit..."), p);
        layout->addWidget(buttonEditSuggestion, 1, 1, 1, 1);
        // TODO no functionality yet, disable button
        buttonEditSuggestion->setEnabled(false);

        buttonDeleteSuggestion = new KPushButton(KIcon("list-remove"), i18n("Delete"), p);
        layout->addWidget(buttonDeleteSuggestion, 2, 1, 1, 1);
        // TODO no functionality yet, disable button
        buttonDeleteSuggestion->setEnabled(false);

        buttonSuggestionUp = new KPushButton(KIcon("go-up"), i18n("Up"), p);
        layout->addWidget(buttonSuggestionUp, 3, 1, 1, 1);

        buttonSuggestionDown = new KPushButton(KIcon("go-down"), i18n("Down"), p);
        layout->addWidget(buttonSuggestionDown, 4, 1, 1, 1);

        connect(buttonSuggestionUp, SIGNAL(clicked()), p, SLOT(buttonClicked()));
        connect(buttonSuggestionDown, SIGNAL(clicked()), p, SLOT(buttonClicked()));
    }
};


SettingsIdSuggestionsWidget::SettingsIdSuggestionsWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsIdSuggestionsWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

SettingsIdSuggestionsWidget::~SettingsIdSuggestionsWidget()
{
    delete d;
}

void SettingsIdSuggestionsWidget::loadState()
{
    d->loadState();
}

void SettingsIdSuggestionsWidget::saveState()
{
    d->saveState();
}

void SettingsIdSuggestionsWidget::resetToDefaults()
{
    d->resetToDefaults();
}

void SettingsIdSuggestionsWidget::buttonClicked()
{
    KPushButton *button = dynamic_cast<KPushButton*>(sender());
    QModelIndex selectedIndex = d->treeViewSuggestions->selectionModel()->currentIndex();

    if (button == d->buttonNewSuggestion) {
        // TODO
    } else if (button == d->buttonEditSuggestion) {
        // TODO
    } else if (button == d->buttonDeleteSuggestion) {
        // TODO
    } else if (button == d->buttonSuggestionUp) {
        if (d->idSuggestionsModel->moveUp(selectedIndex)) {
            d->treeViewSuggestions->selectionModel()->setCurrentIndex(selectedIndex.sibling(selectedIndex.row() - 1, 0), QItemSelectionModel::ClearAndSelect);
            emit changed();
        }
    } else if (button == d->buttonSuggestionDown) {
        if (d->idSuggestionsModel->moveDown(selectedIndex)) {
            d->treeViewSuggestions->selectionModel()->setCurrentIndex(selectedIndex.sibling(selectedIndex.row() + 1, 0), QItemSelectionModel::ClearAndSelect);
            emit changed();
        }
    }
}

void SettingsIdSuggestionsWidget::itemChanged(const QModelIndex &index)
{
    bool enableChange = index != QModelIndex();
// TODO no functionality yet, disable button
//d->buttonEditSuggestion->setEnabled(enableChange);
// TODO no functionality yet, disable button
//d->buttonDeleteSuggestion->setEnabled(enableChange);
    d->buttonSuggestionUp->setEnabled(enableChange && index.row() > 0);
    d->buttonSuggestionDown->setEnabled(enableChange && index.row() < d->idSuggestionsModel->rowCount() - 1);
}
