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

#include "settingsidsuggestionswidget.h"

#include <QLayout>
#include <QTreeView>
#include <QPushButton>
#include <QFontDatabase>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>

#include "fileimporterbibtex.h"
#include "idsuggestions.h"
#include "settingsidsuggestionseditor.h"

class IdSuggestionsModel : public QAbstractListModel
{
    Q_OBJECT

private:
    QStringList m_formatStringList;
    int m_defaultFormatStringRow;
    IdSuggestions *m_idSuggestions;
    static const QString exampleBibTeXEntryString;
    static QSharedPointer<const Entry> exampleBibTeXEntry;

public:
    IdSuggestionsModel(QObject *parent = nullptr)
            : QAbstractListModel(parent) {
        m_idSuggestions = new IdSuggestions();
        m_defaultFormatStringRow = -1;

        if (exampleBibTeXEntry.isNull()) {
            FileImporterBibTeX fileImporterBibTeX(this);
            File *file = fileImporterBibTeX.fromString(exampleBibTeXEntryString);
            if (file != nullptr) {
                if (!file->isEmpty())
                    exampleBibTeXEntry = file->first().dynamicCast<const Entry>();
                delete file;
            }
        }
    }

    ~IdSuggestionsModel() override {
        delete m_idSuggestions;
    }

    enum IdSuggestionsModelRole {
        /// Raw format string as QString
        FormatStringRole = Qt::UserRole + 7811,
        /// Flag whether current index is the default one; boolean value
        IsDefaultFormatStringRole = Qt::UserRole + 7812
    };

    QSharedPointer<const Entry> previewEntry() {
        return exampleBibTeXEntry;
    }

    void setFormatStringList(const QStringList &formatStringList, const QString &defaultString = QString()) {
        beginResetModel();
        m_formatStringList = formatStringList;
        m_defaultFormatStringRow = m_formatStringList.indexOf(defaultString);
        endResetModel();
    }

    QStringList formatStringList() const {
        return this->m_formatStringList;
    }

    QString defaultFormatString() const {
        if (m_defaultFormatStringRow >= 0 && m_defaultFormatStringRow < m_formatStringList.length())
            return m_formatStringList[m_defaultFormatStringRow];
        else
            return QString();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override {
        if (parent == QModelIndex())
            return m_formatStringList.count();
        else
            return 0;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (index.row() < 0 || index.row() >= m_formatStringList.count())
            return QVariant();

        switch (role) {
        case Qt::FontRole: {
            QFont defaultFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
            if (index.row() == m_defaultFormatStringRow)
                defaultFont.setBold(true);
            return defaultFont;
        }
        case Qt::DecorationRole:
            if (index.row() == m_defaultFormatStringRow)
                return QIcon::fromTheme(QStringLiteral("favorites"));
            else
                return QIcon::fromTheme(QStringLiteral("view-filter"));
        case Qt::ToolTipRole:
            return i18n("<qt>Structure:<ul><li>%1</li></ul>Example: %2</qt>", m_idSuggestions->formatStrToHuman(m_formatStringList[index.row()]).join(QStringLiteral("</li><li>")), m_idSuggestions->formatId(*exampleBibTeXEntry, m_formatStringList[index.row()]));
        case Qt::DisplayRole:
            return m_idSuggestions->formatId(*exampleBibTeXEntry, m_formatStringList[index.row()]);
        case Qt::UserRole:
        case FormatStringRole:
            return m_formatStringList[index.row()];
        case IsDefaultFormatStringRole:
            return index.row() == m_defaultFormatStringRow;
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex &idx, const QVariant &value, int role) override {
        if (idx.row() < 0 || idx.row() >= m_formatStringList.count())
            return false;

        if (role == IsDefaultFormatStringRole && value.canConvert<bool>()) {
            if (value.toBool()) {
                if (idx.row() != m_defaultFormatStringRow) {
                    QModelIndex oldDefaultIndex = index(m_defaultFormatStringRow, 0);
                    m_defaultFormatStringRow = idx.row();
                    emit dataChanged(oldDefaultIndex, oldDefaultIndex);
                    emit dataChanged(idx, idx);
                }
            } else {
                m_defaultFormatStringRow = -1;
                emit dataChanged(idx, idx);
            }

            return true;
        } else if (role == FormatStringRole && value.canConvert<QString>()) {
            m_formatStringList[idx.row()] = value.toString();
            emit dataChanged(idx, idx);
            return true;
        }

        return false;
    }

    virtual bool insertRow(int row, const QModelIndex &parent = QModelIndex()) {
        if (parent != QModelIndex()) return false;

        beginInsertRows(parent, row, row);
        m_formatStringList.insert(row, QStringLiteral("T"));
        endInsertRows();

        return true;
    }

    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && section == 0)
            return i18n("Id Suggestions");

        return QVariant();
    }

    bool moveUp(const QModelIndex &index) {
        int row = index.row();
        if (row < 1 || row >= m_formatStringList.count())
            return false;

        beginMoveColumns(index.parent(), row, row, index.parent(), row - 1);
        const QString formatString = m_formatStringList[row];
        m_formatStringList.removeAt(row);
        m_formatStringList.insert(row - 1, formatString);
        if (m_defaultFormatStringRow == row) --m_defaultFormatStringRow; ///< update default id suggestion
        endMoveRows();

        return true;
    }

    bool moveDown(const QModelIndex &index) {
        int row = index.row();
        if (row < 0 || row >= m_formatStringList.count() - 1)
            return false;

        beginMoveColumns(index.parent(), row + 1, row + 1, index.parent(), row);
        const QString formatString = m_formatStringList[row];
        m_formatStringList.removeAt(row);
        m_formatStringList.insert(row + 1, formatString);
        if (m_defaultFormatStringRow == row) ++m_defaultFormatStringRow; ///< update default id suggestion
        endMoveRows();

        return true;
    }

    bool remove(const QModelIndex &index) {
        int row = index.row();
        if (row < 0 || row >= m_formatStringList.count())
            return false;

        beginRemoveRows(index.parent(), row, row);
        m_formatStringList.removeAt(row);
        if (m_defaultFormatStringRow == row) m_defaultFormatStringRow = -1; ///< update default id suggestion
        endRemoveRows();

        return true;
    }
};

const QString IdSuggestionsModel::exampleBibTeXEntryString = QStringLiteral("@Article{ dijkstra1983terminationdetect,\nauthor = {Edsger W. Dijkstra and W. H. J. Feijen and A. J. M. {van Gasteren}},\ntitle = {{Derivation of a Termination Detection Algorithm for Distributed Computations}},\njournal = {Information Processing Letters},\nvolume = 16,\nnumber = 5,\npages = {217--219},\nmonth = jun,\nyear = 1983\n}");
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
    QPushButton *buttonNewSuggestion, *buttonEditSuggestion, *buttonDeleteSuggestion, *buttonSuggestionUp, *buttonSuggestionDown, *buttonToggleDefaultString;

    SettingsIdSuggestionsWidgetPrivate(SettingsIdSuggestionsWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroup(config, IdSuggestions::configGroupName) {
        setupGUI();
    }

    void loadState() {
        idSuggestionsModel->setFormatStringList(configGroup.readEntry(IdSuggestions::keyFormatStringList, IdSuggestions::defaultFormatStringList), configGroup.readEntry(IdSuggestions::keyDefaultFormatString, IdSuggestions::defaultDefaultFormatString));
    }

    void saveState() {
        configGroup.writeEntry(IdSuggestions::keyFormatStringList, idSuggestionsModel->formatStringList());
        configGroup.writeEntry(IdSuggestions::keyDefaultFormatString, idSuggestionsModel->defaultFormatString());
        config->sync();
    }

    void resetToDefaults() {
        idSuggestionsModel->setFormatStringList(IdSuggestions::defaultFormatStringList);
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        treeViewSuggestions = new QTreeView(p);
        layout->addWidget(treeViewSuggestions, 0, 0, 8, 1);
        idSuggestionsModel = new IdSuggestionsModel(treeViewSuggestions);
        treeViewSuggestions->setModel(idSuggestionsModel);
        treeViewSuggestions->setRootIsDecorated(false);
        connect(treeViewSuggestions->selectionModel(), &QItemSelectionModel::currentChanged, p, &SettingsIdSuggestionsWidget::itemChanged);
        treeViewSuggestions->setMinimumSize(treeViewSuggestions->fontMetrics().width(QChar('W')) * 25, treeViewSuggestions->fontMetrics().height() * 15);

        buttonNewSuggestion = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add..."), p);
        layout->addWidget(buttonNewSuggestion, 0, 1, 1, 1);

        buttonEditSuggestion = new QPushButton(QIcon::fromTheme(QStringLiteral("document-edit")), i18n("Edit..."), p);
        layout->addWidget(buttonEditSuggestion, 1, 1, 1, 1);

        buttonDeleteSuggestion = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove"), p);
        layout->addWidget(buttonDeleteSuggestion, 2, 1, 1, 1);

        buttonSuggestionUp = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), i18n("Up"), p);
        layout->addWidget(buttonSuggestionUp, 3, 1, 1, 1);

        buttonSuggestionDown = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down")), i18n("Down"), p);
        layout->addWidget(buttonSuggestionDown, 4, 1, 1, 1);

        buttonToggleDefaultString = new QPushButton(QIcon::fromTheme(QStringLiteral("favorites")), i18n("Toggle Default"), p);
        layout->addWidget(buttonToggleDefaultString, 5, 1, 1, 1);

        connect(buttonNewSuggestion, &QPushButton::clicked, p, &SettingsIdSuggestionsWidget::buttonClicked);
        connect(buttonEditSuggestion, &QPushButton::clicked, p, &SettingsIdSuggestionsWidget::buttonClicked);
        connect(buttonDeleteSuggestion, &QPushButton::clicked, p, &SettingsIdSuggestionsWidget::buttonClicked);
        connect(buttonSuggestionUp, &QPushButton::clicked, p, &SettingsIdSuggestionsWidget::buttonClicked);
        connect(buttonSuggestionDown, &QPushButton::clicked, p, &SettingsIdSuggestionsWidget::buttonClicked);
        connect(buttonToggleDefaultString, &QPushButton::clicked, p, &SettingsIdSuggestionsWidget::toggleDefault);
        connect(treeViewSuggestions, &QTreeView::doubleClicked, p, &SettingsIdSuggestionsWidget::editItem);
    }
};


SettingsIdSuggestionsWidget::SettingsIdSuggestionsWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsIdSuggestionsWidgetPrivate(this))
{
    d->loadState();
    itemChanged(QModelIndex());
}

SettingsIdSuggestionsWidget::~SettingsIdSuggestionsWidget()
{
    delete d;
}

QString SettingsIdSuggestionsWidget::label() const
{
    return i18n("Id Suggestions");
}

QIcon SettingsIdSuggestionsWidget::icon() const
{
    return QIcon::fromTheme(QStringLiteral("view-filter"));
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
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    QModelIndex selectedIndex = d->treeViewSuggestions->selectionModel()->currentIndex();

    if (button == d->buttonNewSuggestion) {
        const QString newSuggestion = IdSuggestionsEditDialog::editSuggestion(d->idSuggestionsModel->previewEntry().data(), QString(), this);
        const int row = d->treeViewSuggestions->model()->rowCount(QModelIndex());
        if (!newSuggestion.isEmpty() && d->idSuggestionsModel->insertRow(row)) {
            QModelIndex index = d->idSuggestionsModel->index(row, 0, QModelIndex());
            d->treeViewSuggestions->setCurrentIndex(index);
            if (d->idSuggestionsModel->setData(index, newSuggestion, IdSuggestionsModel::FormatStringRole))
                emit changed();
        }
    } else if (button == d->buttonEditSuggestion) {
        QModelIndex currIndex = d->treeViewSuggestions->currentIndex();
        editItem(currIndex);
    } else if (button == d->buttonDeleteSuggestion) {
        if (d->idSuggestionsModel->remove(selectedIndex)) {
            emit changed();
        }
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
    d->buttonEditSuggestion->setEnabled(enableChange);
    d->buttonDeleteSuggestion->setEnabled(enableChange);
    d->buttonSuggestionUp->setEnabled(enableChange && index.row() > 0);
    d->buttonSuggestionDown->setEnabled(enableChange && index.row() < d->idSuggestionsModel->rowCount() - 1);

    d->buttonToggleDefaultString->setEnabled(enableChange);
}

void SettingsIdSuggestionsWidget::editItem(const QModelIndex &index)
{
    QString suggestion;
    if (index != QModelIndex() && !(suggestion = index.data(IdSuggestionsModel::FormatStringRole).toString()).isEmpty()) {
        const QString newSuggestion = IdSuggestionsEditDialog::editSuggestion(d->idSuggestionsModel->previewEntry().data(), suggestion, this);
        if (newSuggestion.isEmpty()) {
            if (KMessageBox::questionYesNo(this, i18n("All token have been removed from this suggestion. Remove suggestion itself or restore original suggestion?"), i18n("Remove suggestion?"), KGuiItem(i18n("Remove suggestion"), QIcon::fromTheme(QStringLiteral("list-remove"))), KGuiItem(i18n("Revert changes"), QIcon::fromTheme(QStringLiteral("edit-undo")))) == KMessageBox::Yes && d->idSuggestionsModel->remove(index)) {
                emit changed();
            }
        } else if (newSuggestion != suggestion) {
            if (d->idSuggestionsModel->setData(index, newSuggestion, IdSuggestionsModel::FormatStringRole))
                emit changed();
        }
    }

}

void SettingsIdSuggestionsWidget::toggleDefault()
{
    QModelIndex curIndex = d->treeViewSuggestions->currentIndex();
    bool current = d->idSuggestionsModel->data(curIndex, IdSuggestionsModel::IsDefaultFormatStringRole).toBool();
    d->idSuggestionsModel->setData(curIndex, !current, IdSuggestionsModel::IsDefaultFormatStringRole);
    emit changed();
}

#include "settingsidsuggestionswidget.moc"
