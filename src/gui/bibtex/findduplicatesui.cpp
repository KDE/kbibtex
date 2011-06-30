/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QApplication>
#include <QWidget>
#include <QBoxLayout>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QRadioButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSplitter>

#include <KPushButton>
#include <KAction>
#include <KDialog>
#include <KActionCollection>
#include <KLocale>
#include <KXMLGUIClient>
#include <KStandardDirs>
#include <kparts/part.h>
#include <KMessageBox>
#include <KInputDialog>

#include <kdeversion.h>

#include <radiobuttontreeview.h>
#include "bibtexeditor.h"
#include "bibtexfilemodel.h"
#include "findduplicatesui.h"
#include "findduplicates.h"
#include "bibtexentries.h"

const int FieldNameRole = Qt::UserRole + 101;

const int maxFieldsCount = 1024;

/**
 * Model to hold alternative values as visualized in the RadioTreeView
 */
class AlternativesItemModel : public QAbstractItemModel
{
private:
    /// marker to memorize in an index's internal id that it is a top-level index
    static const quint32 noParentInternalId;

    /// parent widget, needed to get font from (for text in italics)
    QTreeView *p;

    EntryClique *currentClique;

public:
    AlternativesItemModel(QTreeView *parent)
            : QAbstractItemModel(parent), p(parent), currentClique(NULL) {
        // nothing
    }

    void setCurrentClique(EntryClique *currentClique) {
        this->currentClique = currentClique;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const {
        if (parent == QModelIndex())
            return createIndex(row, column, noParentInternalId);
        else if (parent.parent() == QModelIndex())
            return createIndex(row, column, parent.row());
        return QModelIndex();
    }

    QModelIndex parent(const QModelIndex &index) const {
        if (index.internalId() >= noParentInternalId)
            return QModelIndex();
        else
            return createIndex(index.internalId(), 0, noParentInternalId);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const {
        if (currentClique == NULL)
            return 0;

        if (parent == QModelIndex()) {
            /// top-level index, check how many lists of lists of alternatives exist
            return currentClique->fieldCount();
        } else if (parent.parent() == QModelIndex()) {
            /// first, find the map of alternatives for this chosen field name (see parent)
            QString fieldName = parent.data(FieldNameRole).toString();
            QList<Value> alt = currentClique->values(fieldName);
            /// second, return number of alternatives for list of alternatives
            /// plus one for an "else" option
            return alt.count() + (fieldName.startsWith('^') || fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl ? 0 : 1);
        }

        return 0;
    }

    int columnCount(const QModelIndex &) const {
        /// only one column in use
        return 1;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const {
        Q_UNUSED(section)
        Q_UNUSED(orientation)

        if (role == Qt::DisplayRole)
            return i18n("Alternatives");
        return QVariant();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
        if (index.parent() == QModelIndex()) {
            /// top-level elements showing field names like "Title", "Authors", etc
            const QString fieldName = currentClique->fieldList().at(index.row()).toLower();
            switch (role) {
            case FieldNameRole:
                /// plain-and-simple field name (all lower case)
                return fieldName;
            case Qt::ToolTipRole:
            case Qt::DisplayRole:
                /// nicely formatted field names for visual representation
                if (fieldName == QLatin1String("^id"))
                    return i18n("Identifier");
                else if (fieldName == QLatin1String("^type"))
                    return i18n("Type");
                else
                    return BibTeXEntries::self()->format(fieldName, KBibTeX::cUpperCamelCase);
            case IsRadioRole:
                /// this is not to be a radio widget
                return QVariant::fromValue(false);
            case Qt::FontRole:
                if (fieldName.startsWith('^')) {
                    QFont f = p->font();
                    f.setItalic(true);
                    return f;
                }
            }
        } else if (index.parent().parent() == QModelIndex()) {
            /// second-level entries for alternatives

            /// start with determining which list of alternatives actually to use
            QString fieldName = index.parent().data(FieldNameRole).toString();
            QList<Value> values = currentClique->values(fieldName);

            switch (role) {
            case Qt::ToolTipRole:
            case Qt::DisplayRole:
                if (index.row() < values.count()) {
                    QString text = PlainTextValue::text(values.at(index.row()));
                    if (fieldName == QLatin1String("^type")) {
                        BibTeXEntries *be = BibTeXEntries::self();
                        text = be->format(text, KBibTeX::cUpperCamelCase);
                    }

                    /// textual representation of the alternative's value
                    return text;
                } else
                    /// add an "else" option
                    return i18n("None of the above");
            case Qt::FontRole:
                /// for the "else" option, make font italic
                if (index.row() >= values.count()) {
                    QFont f = p->font();
                    f.setItalic(true);
                    return f;
                }
            case Qt::CheckStateRole: {
                if (fieldName != Entry::ftKeywords && fieldName != Entry::ftUrl)
                    return QVariant();

                QList<Value> chosenValues = currentClique->chosenValues(fieldName);
                QString text = PlainTextValue::text(values.at(index.row()));
                foreach(Value value, chosenValues) {
                    if (PlainTextValue::text(value) == text)
                        return Qt::Checked;
                }

                return Qt::Unchecked;
            }
            case RadioSelectedRole: {
                if (fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl)
                    return QVariant::fromValue(false);

                /// return selection status (true or false) for this alternative
                Value chosen = currentClique->chosenValue(fieldName);
                if (chosen.isEmpty())
                    return QVariant::fromValue(index.row() >= values.count());
                else if (index.row() < values.count()) {
                    QString chosenPlainText = PlainTextValue::text(chosen);
                    QString rowPlainText = PlainTextValue::text(values[index.row()]);
                    return QVariant::fromValue(chosenPlainText == rowPlainText);
                }
                return QVariant::fromValue(false);
            }
            case IsRadioRole:
                /// this is to be a radio widget
                return QVariant::fromValue(fieldName != Entry::ftKeywords && fieldName != Entry::ftUrl);
            }
        }

        return QVariant();
    }

    bool setData(const QModelIndex & index, const QVariant & value, int role = RadioSelectedRole) {
        if (index.parent() != QModelIndex()) {
            QString fieldName = index.parent().data(FieldNameRole).toString();
            if (role == RadioSelectedRole && value.canConvert<bool>() && value.toBool() == true) {
                /// start with determining which list of alternatives actually to use
                QList<Value> values = currentClique->values(fieldName);

                /// store which alternative was selected
                if (index.row() < values.count())
                    currentClique->setChosenValue(fieldName, values[index.row()]);
                else {
                    Value v;
                    currentClique->setChosenValue(fieldName, v);
                }

                /// update view on neighbouring alternatives
                emit dataChanged(index.sibling(0, 0), index.sibling(rowCount(index.parent()), 0));

                return true;
            } else if (role == Qt::CheckStateRole && (fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl)) {
                bool ok;
                int checkState = value.toInt(&ok);
                if (ok) {
                    QList<Value> values = currentClique->values(fieldName);
                    if (checkState == Qt::Checked)
                        currentClique->setChosenValue(fieldName, values[index.row()], EntryClique::AddValue);
                    else if (checkState == Qt::Unchecked)
                        currentClique->setChosenValue(fieldName, values[index.row()], EntryClique::RemoveValue);

                    return true;
                }
            }
        }

        return false;
    }

    bool hasChildren(const QModelIndex & parent = QModelIndex()) const {
        /// depth-two tree
        return parent == QModelIndex() || parent.parent() == QModelIndex();
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const {
        Qt::ItemFlags f = QAbstractItemModel::flags(index);
        if (index.parent() != QModelIndex()) {
            QString fieldName = index.parent().data(FieldNameRole).toString();
            if (fieldName == Entry::ftKeywords || fieldName == Entry::ftUrl)
                f |= Qt::ItemIsUserCheckable;
        }
        return f;
    }
};

const quint32 AlternativesItemModel::noParentInternalId = 0xffffff;


class CheckableBibTeXFileModel : public BibTeXFileModel
{
private:
    QList<EntryClique*> cl;
    int currentClique;
    QTreeView *tv;

public:
    CheckableBibTeXFileModel(QList<EntryClique*> &cliqueList, QTreeView *treeView, QObject *parent = NULL)
            : BibTeXFileModel(parent), cl(cliqueList), currentClique(0), tv(treeView) {
        // nothing
    }

    void setCurrentClique(EntryClique *currentClique) {
        this->currentClique = cl.indexOf(currentClique);
    }

    virtual QVariant data(const QModelIndex &index, int role) const {
        if (role == Qt::CheckStateRole && index.column() == 1) {
            Entry *entry = dynamic_cast<Entry*>(element(index.row()));
            Q_ASSERT_X(entry != NULL, "CheckableBibTeXFileModel::data", "entry is NULL");
            if (entry != NULL) {
                QList<Entry*> entryList = cl[currentClique]->entryList();
                if (entryList.contains(entry))
                    return cl[currentClique]->isEntryChecked(entry) ? Qt::Checked : Qt::Unchecked;
            }
        }

        return BibTeXFileModel::data(index, role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {
        bool ok;
        int checkState = value.toInt(&ok);
        Q_ASSERT_X(ok, "CheckableBibTeXFileModel::setData", QString("Could not convert value " + value.toString()).toAscii());
        if (ok && role == Qt::CheckStateRole && index.column() == 1) {
            Entry *entry = dynamic_cast<Entry*>(element(index.row()));
            if (entry != NULL) {
                QList<Entry*> entryList = cl[currentClique]->entryList();
                if (entryList.contains(entry)) {
                    EntryClique *ec = cl[currentClique];
                    ec->setEntryChecked(entry, checkState == Qt::Checked);
                    cl[currentClique] = ec;
                    emit dataChanged(index, index);
                    tv->reset();
                    return true;
                }
            }
        }

        return false;
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const {
        Qt::ItemFlags f = BibTeXFileModel::flags(index);
        if (index.column() == 1)
            f |= Qt::ItemIsUserCheckable;
        return f;
    }
};


class FilterIdBibTeXFileModel : public QSortFilterProxyModel
{
private:
    CheckableBibTeXFileModel *internalModel;
    EntryClique* currentClique;

public:
    FilterIdBibTeXFileModel(QObject *parent = NULL)
            : QSortFilterProxyModel(parent), internalModel(NULL), currentClique(NULL) {
        // nothing
    }

    void setCurrentClique(EntryClique* currentClique) {
        Q_ASSERT(internalModel != NULL);
        internalModel->setCurrentClique(currentClique);
        this->currentClique = currentClique;
        invalidateFilter();
    }

    void setSourceModel(QAbstractItemModel *model) {
        QSortFilterProxyModel::setSourceModel(model);
        internalModel = dynamic_cast<CheckableBibTeXFileModel*>(model);
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
        Q_UNUSED(source_parent)

        if (internalModel != NULL && currentClique != NULL) {
            Entry *entry = dynamic_cast<Entry*>(internalModel->element(source_row));
            if (entry != NULL) {
                QList<Entry*> entryList = currentClique->entryList();
                if (entryList.contains(entry)) return true;
            }
        }
        return false;
    }
};


class MergeWidget::MergeWidgetPrivate
{
private:
    MergeWidget *p;

public:
    File *file;
    BibTeXEditor *editor;
    KPushButton *buttonNext, *buttonPrev;

    CheckableBibTeXFileModel *model;
    FilterIdBibTeXFileModel *filterModel;

    RadioButtonTreeView *alternativesView;
    AlternativesItemModel *alternativesItemModel;

    int currentClique;
    QList<EntryClique*> cl;

    MergeWidgetPrivate(MergeWidget *parent, QList<EntryClique*> &cliqueList)
            : p(parent), currentClique(0), cl(cliqueList) {
        // nothing
    }

    void setupGUI() {
        p->setMinimumSize(p->fontMetrics().xHeight()*96, p->fontMetrics().xHeight()*64);
        p->setBaseSize(p->fontMetrics().xHeight()*128, p->fontMetrics().xHeight()*96);

        QBoxLayout *layout = new QVBoxLayout(p);

        QLabel *label = new QLabel(i18n("Select your duplicates"), p);
        layout->addWidget(label);

        QSplitter *splitter = new QSplitter(Qt::Vertical, p);
        layout->addWidget(splitter);

        editor = new BibTeXEditor(QLatin1String("MergeWidget"), splitter);
        editor->setReadOnly(true);

        alternativesView = new RadioButtonTreeView(splitter);

        model = new CheckableBibTeXFileModel(cl, alternativesView, p);
        model->setBibTeXFile(new File(*file));

        QBoxLayout *containerLayout = new QHBoxLayout();
        layout->addLayout(containerLayout);
        buttonPrev = new KPushButton(KIcon("go-previous"), i18n("Previous"), p);
        containerLayout->addWidget(buttonPrev, 1);
        containerLayout->addStretch(10);
        buttonNext = new KPushButton(KIcon("go-next"), i18n("Next"), p);
        containerLayout->addWidget(buttonNext, 1);

        filterModel = new FilterIdBibTeXFileModel(p);
        filterModel->setSourceModel(model);
        alternativesItemModel = new AlternativesItemModel(alternativesView);

        showCurrentClique();

        connect(buttonPrev, SIGNAL(clicked()), p, SLOT(previousClique()));
        connect(buttonNext, SIGNAL(clicked()), p, SLOT(nextClique()));

        connect(editor, SIGNAL(doubleClicked(QModelIndex)), editor, SLOT(viewCurrentElement()));
    }

    void showCurrentClique() {
        EntryClique *ec = cl[currentClique];
        filterModel->setCurrentClique(ec);
        alternativesItemModel->setCurrentClique(ec);
        editor->setModel(filterModel);
        alternativesView->setModel(alternativesItemModel);
        editor->reset();
        alternativesView->reset();
        alternativesView->expandAll();

        buttonNext->setEnabled(currentClique >= 0 && currentClique < cl.count() - 1);
        buttonPrev->setEnabled(currentClique > 0);
    }

};

MergeWidget::MergeWidget(File *file, QList<EntryClique*> &cliqueList, QWidget *parent)
        : QWidget(parent), d(new MergeWidgetPrivate(this, cliqueList))
{
    d->file = file;
    d->setupGUI();
}

void MergeWidget::previousClique()
{
    if (d->currentClique > 0) {
        --d->currentClique;
        d->showCurrentClique();
    }
}

void MergeWidget::nextClique()
{
    if (d->currentClique >= 0 && d->currentClique < d->cl.count() - 1) {
        ++d->currentClique;
        d->showCurrentClique();
    }
}


class FindDuplicatesUI::FindDuplicatesUIPrivate
{
private:
    FindDuplicatesUI *p;

public:
    KParts::Part *part;
    BibTeXEditor *editor;

    FindDuplicatesUIPrivate(FindDuplicatesUI *parent, KParts::Part *kpart, BibTeXEditor *bibTeXEditor)
            : p(parent), part(kpart), editor(bibTeXEditor) {
        // nothing
    }
};

FindDuplicatesUI::FindDuplicatesUI(KParts::Part *part, BibTeXEditor *bibTeXEditor)
        : QObject(), d(new FindDuplicatesUIPrivate(this, part, bibTeXEditor))
{
    KAction *newAction = new KAction(KIcon("tab-duplicate"), i18n("Find Duplicates"), this);
    part->actionCollection()->addAction(QLatin1String("findduplicates"), newAction);
    connect(newAction, SIGNAL(triggered()), this, SLOT(slotFindDuplicates()));
#if KDE_VERSION_MINOR >= 4
    part->replaceXMLFile(KStandardDirs::locate("appdata", "findduplicatesui.rc"), KStandardDirs::locateLocal("appdata", "findduplicatesui.rc"), true);
#endif
}

void FindDuplicatesUI::slotFindDuplicates()
{
    bool ok = false;
    int sensitivity = KInputDialog::getInteger(i18n("Sensitivity"), i18n("Enter a value for sensitivity.\n\nLow values (close to 0) require very similar entries for duplicate detection, larger values (close to 10000) are more likely to count entries as duplicates.\n\nPlease provide feedback to the developers if you have a suggestion for a better default value than 4000."), 4000, 0, 10000, 10, &ok, d->part->widget());
    if (!ok) sensitivity = 4000;

    KDialog dlg(d->part->widget());
    FindDuplicates fd(&dlg, sensitivity);
    File *file = d->editor->bibTeXModel()->bibTeXFile();
    bool deleteFileLater = false;

    int rowCount = d->editor->selectedElements().count() / d->editor->model()->columnCount();
    if (rowCount > 1 && rowCount < d->editor->model()->rowCount() && KMessageBox::questionYesNo(d->part->widget(), i18n("Multiple elements are selected. Do you want to search for duplicates only within the selection or in the whole document?"), i18n("Search only in selection?"), KGuiItem(i18n("Only in selection")), KGuiItem(i18n("Whole document"))) == KMessageBox::Yes) {
        QModelIndexList mil = d->editor->selectionModel()->selectedRows();
        file = new File();
        deleteFileLater = true;
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
            file->append(d->editor->bibTeXModel()->element(d->editor->sortFilterProxyModel()->mapToSource(*it).row()));
        }
    }

    QList<EntryClique*> cliques;
    bool gotCanceled = fd.findDuplicateEntries(file, cliques);
    if (gotCanceled) {
        if (deleteFileLater) delete file;
        return;
    }

    if (cliques.isEmpty()) {
        KMessageBox::information(d->part->widget(), i18n("No duplicates have been found."), i18n("No duplicates found"));
    } else {
        MergeWidget mw(d->editor->bibTeXModel()->bibTeXFile(), cliques, &dlg);
        dlg.setMainWidget(&mw);

        if (dlg.exec() == QDialog::Accepted) {
            MergeDuplicates md(&dlg);
            file = d->editor->bibTeXModel()->bibTeXFile();
            if (md.mergeDuplicateEntries(cliques, file)) {
                d->editor->bibTeXModel()->setBibTeXFile(file);
            }
        }
    }

    if (deleteFileLater) delete file;
}
