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

#include <KAction>
#include <KDialog>
#include <KActionCollection>
#include <KDebug>
#include <KLocale>
#include <KXMLGUIClient>
#include <KStandardDirs>
#include <kparts/part.h>

#include "bibtexeditor.h"
#include "bibtexfilemodel.h"
#include "findduplicatesui.h"
#include "findduplicates.h"
#include "bibtexentries.h"

/// return QString to get the field name of an alternative (e.g. "title" or "volume")
const int FieldNameRole = Qt::UserRole + 101;
/// return bool to know if an alternative is selected or not
const int RadioSelectionRole = Qt::UserRole + 102;
/// maximum number of different fields in all alternatives; set to 1024 by default
const int maxFieldsCount = 1024;

/**
  * Structure to memorize the possible alternatives for a given field name.
  */
typedef struct {
    /// field name, e.g. "title" or "volume"
    QString fieldName;
    /// number of alternatives as refered to by the pointer below
    int alternativesCount;
    /// pointer to a list of alternative values for this field
    Value *alternatives;
    /// selected alternative (0 .. alternativesCount-1)
    int selectedAlternative;
} AlternativesListItem;

class AlternativesItemModel : public QAbstractItemModel, public QList<Entry*>
{
private:
    /// marker to memorize in an index's internal id that it is a top-level index
    static const quint32 noParentInternalId;

    /// BibTeX file to operate on
    File *file;
    /// parent widget, needed to get font from (for text in italics)
    QWidget *p;
    /// fixed-size list of alternative items;
    /// number of elements in array determined by alternativesListCount
    AlternativesListItem alternativesList[maxFieldsCount];
    /// number of actual elements in the array above
    int alternativesListCount;

    /**
     * Update the internal representation of the duplicates in Entry items in this model.
     */
    void updateInternalModel() {
        /// delete old array and reset everything
        for (int i = 0; i < alternativesListCount; ++i)
            delete[] alternativesList[i].alternatives;
        alternativesListCount = 0;

        /// go through each and every entry ...
        for (QList<Entry*>::ConstIterator dupsIt = constBegin(); dupsIt != constEnd(); ++dupsIt) {
            const Entry *entry = *dupsIt;

            /// go through each and every field of this entry
            for (Entry::ConstIterator fieldIt = entry->constBegin(); fieldIt != entry->constEnd(); ++fieldIt) {
                /// store both field name and value for later reference
                const QString fieldName = fieldIt.key().toLower();
                const Value fieldValue = fieldIt.value();
                const QString fieldValueText = PlainTextValue::text(fieldValue);

                /// go through list of list of alternatives if there is already
                /// a list of alternatives for this field name
                int altListIndex = alternativesListCount - 1;
                for (; altListIndex >= 0; --altListIndex)
                    if (alternativesList[altListIndex].fieldName == fieldName)
                        break;

                if (altListIndex < 0) {
                    /// no, there is no such list of alternatives
                    /// initialize a new list of alternatives
                    altListIndex = alternativesListCount;
                    ++alternativesListCount;
                    alternativesList[altListIndex].fieldName = fieldName;
                    alternativesList[altListIndex].alternativesCount = 0;
                    alternativesList[altListIndex].alternatives = new Value[count()];
                    alternativesList[altListIndex].selectedAlternative = 0;
                }

                /// in the list of alternatives, search of a value identical
                /// to the current (as of fieldIt) value (to avoid duplicates)
                int valueIndex = alternativesList[altListIndex].alternativesCount - 1;
                for (;valueIndex >= 0; --valueIndex)
                    if (PlainTextValue::text(alternativesList[altListIndex].alternatives[valueIndex]) == fieldValueText)
                        break;

                if (valueIndex < 0) {
                    /// no, this value is for this field unique so far
                    /// initialize a new alternative and add it to the list of alternatives
                    valueIndex = alternativesList[altListIndex].alternativesCount;
                    ++alternativesList[altListIndex].alternativesCount;
                    alternativesList[altListIndex].alternatives[valueIndex] = fieldValue;
                }
            }
        }
    }

public:
    AlternativesItemModel(File *bibtexFile, QWidget *parent)
            : QAbstractItemModel(parent), file(bibtexFile), p(parent), alternativesListCount(0) {
        // nothing
    }

    virtual void append(Entry* newValue) {
        /// when adding new Entry items to this list, updated internal model as well
        QList<Entry*>::append(newValue);
        updateInternalModel();
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
        if (parent == QModelIndex()) {
            /// top-level index, check how many lists of lists of alternatives exist
            return alternativesListCount;
        } else if (parent.parent() == QModelIndex()) {
            /// first, find the list of alternatives for this chosen field name (see parent)
            QString fieldName = parent.data(FieldNameRole).toString();
            if (!fieldName.isEmpty()) {
                int index = alternativesListCount - 1;
                for (;index >= 0; --index)
                    if (alternativesList[index].fieldName == fieldName)
                        break;
                /// second, return number of alternatives for list of alternatives
                return index < 0 ? 0 : alternativesList[index].alternativesCount + 1;
            }
        }

        return 0;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const {
        Q_UNUSED(parent)
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
            switch (role) {
            case FieldNameRole:
                /// plain-and-simple field name (all lower case)
                return alternativesList[index.row()].fieldName;
            case Qt::DisplayRole:
                /// nicely formatted field names for visual representation
                return BibTeXEntries::self()->format(alternativesList[index.row()].fieldName, KBibTeX::cUpperCamelCase);
            }
        } else if (index.parent().parent() == QModelIndex()) {
            /// second-level entries for alternatives

            /// start with determining which list of alternatives actually to use
            QString fieldName = index.parent().data(FieldNameRole).toString();
            if (!fieldName.isEmpty()) {
                int altIndex = alternativesListCount - 1;
                for (;altIndex >= 0; --altIndex)
                    if (alternativesList[altIndex].fieldName == fieldName)
                        break;
                /// assume that the data is consistent and the field name will be found
                Q_ASSERT(altIndex >= 0);

                switch (role) {
                case Qt::DisplayRole:
                    if (index.row() < alternativesList[altIndex].alternativesCount)
                        return PlainTextValue::text(alternativesList[altIndex].alternatives[index.row()]);
                    else
                        return i18n("None of the above");
                case Qt::FontRole:
                    if (index.row() >= alternativesList[altIndex].alternativesCount) {
                        QFont f = p->font();
                        f.setItalic(true);
                        return f;
                    }
                case RadioSelectionRole: {
                    return QVariant::fromValue(alternativesList[altIndex].selectedAlternative == index.row());
                }
                }
            }
        }

        return QVariant();
    }

    bool setData(const QModelIndex & index, const QVariant & value, int role = RadioSelectionRole) {
        if (index.parent().parent() == QModelIndex() && role == RadioSelectionRole && value.canConvert<bool>()) {
            QString fieldName = index.parent().data(FieldNameRole).toString();
            if (!fieldName.isEmpty()) {
                int altIndex = alternativesListCount - 1;
                for (;altIndex >= 0; --altIndex)
                    if (alternativesList[altIndex].fieldName == fieldName)
                        break;
                alternativesList[altIndex].selectedAlternative = index.row();

                emit dataChanged(index.sibling(0, 0), index.sibling(rowCount(index.parent()), 0));
                return true;
            }
        }

        return false;
    }

    bool hasChildren(const QModelIndex & parent = QModelIndex()) const {
        /// depth-two tree
        return parent == QModelIndex() || parent.parent() == QModelIndex();
    }
};

const quint32 AlternativesItemModel::noParentInternalId = 0xffffff;

class RadioButtonItemDelegate : public QStyledItemDelegate
{
public:
    RadioButtonItemDelegate(QObject *p)
            : QStyledItemDelegate(p) {
        // nothing
    }

    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
        if (index.parent() != QModelIndex()) {
            int radioButtonWidth = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &option);
            int spacing = QApplication::style()->pixelMetric(QStyle::PM_RadioButtonLabelSpacing, &option);

            QStyleOptionViewItem myOption = option;
            int left = myOption.rect.left();
            myOption.rect.setLeft(left + spacing + radioButtonWidth);
            QStyledItemDelegate::paint(painter, myOption, index);

            myOption.rect.setLeft(left);
            myOption.rect.setWidth(radioButtonWidth);
            if (index.data(RadioSelectionRole).canConvert<bool>()) {
                bool radioButtonSelected = index.data(RadioSelectionRole).value<bool>();
                myOption.state |= radioButtonSelected ? QStyle::State_On : QStyle::State_Off;
            }
            QApplication::style()->drawPrimitive(QStyle::PE_IndicatorRadioButton, &myOption, painter);
        } else
            QStyledItemDelegate::paint(painter, option, index);
    }

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const {
        QSize s = QStyledItemDelegate::sizeHint(option, index);
        if (index.parent() != QModelIndex()) {
            int radioButtonHeight = QApplication::style()->pixelMetric(QStyle::PM_ExclusiveIndicatorHeight, &option);
            s.setHeight(qMax(s.height(), radioButtonHeight));
        }
        return s;
    }
};


class RadioButtonTreeView : public QTreeView
{
public:
    RadioButtonTreeView(QWidget *parent)
            : QTreeView(parent) {
        // nothing
    }

protected:
    void mouseReleaseEvent(QMouseEvent *event) {
        QModelIndex index = indexAt(event->pos());
        if (index != QModelIndex() && index.parent() != QModelIndex()) {
            model()->setData(index, QVariant::fromValue(true), RadioSelectionRole);
            event->accept();
        }
    }

    void keyReleaseEvent(QKeyEvent *event) {
        QModelIndex index = currentIndex();
        if (index != QModelIndex() && index.parent() != QModelIndex() && event->key() == Qt::Key_Space) {
            model()->setData(index, QVariant::fromValue(true), RadioSelectionRole);
            event->accept();
        }
    }
};

class FilterIdBibTeXFileModel : public QSortFilterProxyModel
{
private:
    QStringList allowedIds;
    BibTeXFileModel *internalModel;

public:
    FilterIdBibTeXFileModel(QObject *parent = NULL)
            : QSortFilterProxyModel(parent), internalModel(NULL) {
        // nothing
    }

    void setVisibleIds(const QStringList &newIdList) {
        allowedIds = newIdList;
        invalidateFilter();
    }

    void setSourceModel(QAbstractItemModel *model) {
        QSortFilterProxyModel::setSourceModel(model);
        internalModel = dynamic_cast<BibTeXFileModel*>(model);
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
        Q_UNUSED(source_parent)

        if (internalModel != NULL) {
            Entry *entry = dynamic_cast<Entry*>(internalModel->element(source_row));
            if (entry != NULL)
                return allowedIds.contains(entry->id());
        }
        return false;
    }
};

class CheckableBibTeXFileModel : public BibTeXFileModel
{
private:
    int *checkStatePerRow;

public:
    CheckableBibTeXFileModel(QObject *parent = NULL)
            : BibTeXFileModel(parent), checkStatePerRow(NULL) {
        // nothing
    }

    ~CheckableBibTeXFileModel() {
        if (checkStatePerRow != NULL) delete[] checkStatePerRow;
    }

    void setBibTeXFile(File *file) {
        BibTeXFileModel::setBibTeXFile(file);

        if (checkStatePerRow != NULL) delete[] checkStatePerRow;
        checkStatePerRow = new int[file->count()];
    }

    virtual QVariant data(const QModelIndex &index, int role) const {
        if (role == Qt::CheckStateRole && index.column() == 1) {
            // TODO
            return checkStatePerRow[index.row()];
        } else
            return BibTeXFileModel::data(index, role);
    }

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {
        bool ok;
        int checkState = value.toInt(&ok);
        if (role == Qt::CheckStateRole && index.column() == 1 && ok) {
            kDebug() << "checkState=" << checkState;
            checkStatePerRow[index.row()] = checkState;
            return true;
        } else
            return false;
    }

    virtual Qt::ItemFlags flags(const QModelIndex &index) const {
        Qt::ItemFlags f = BibTeXFileModel::flags(index);
        if (index.column() == 1)
            f |= Qt::ItemIsUserCheckable;
        return f;
    }
};

class MergeWidget : public QWidget
{
private:
    File *file;
    BibTeXEditor *editor;
    RadioButtonTreeView *alternativesView;
    AlternativesItemModel *alternativesItemModel;
    CheckableBibTeXFileModel *model;
    FilterIdBibTeXFileModel *filterModel;
    QList<QList<Entry*> > cliques;

public:
    MergeWidget(File *file, QList<QList<Entry*> > cliques, QWidget *parent)
            : QWidget(parent) {
        this->cliques = cliques;
        this->file = file;

        setMinimumSize(fontMetrics().xHeight()*64, fontMetrics().xHeight()*32);

        QBoxLayout *layout = new QVBoxLayout(this);

        QLabel *label = new QLabel(i18n("Select your duplicates"), this);
        layout->addWidget(label);

        QSplitter *splitter = new QSplitter(Qt::Vertical, this);
        layout->addWidget(splitter);

        editor = new BibTeXEditor(splitter);
        editor->setReadOnly(true);

        model = new CheckableBibTeXFileModel(this);
        model->setBibTeXFile(new File(*file));
        filterModel = new FilterIdBibTeXFileModel(this);
        filterModel->setSourceModel(model);
        editor->setModel(filterModel);

        alternativesView = new RadioButtonTreeView(splitter);
        alternativesItemModel = new AlternativesItemModel(file, alternativesView);
        alternativesView->setModel(alternativesItemModel);
        alternativesView->setItemDelegate(new RadioButtonItemDelegate(alternativesView));

        switchClique(0);
    }

    void switchClique(int i) {
        alternativesItemModel->clear();
        if (i >= 0 && i < cliques.size()) {
            QList<Entry*> entryList = cliques[i];
            QStringList idList;
            for (QList<Entry*>::ConstIterator it = entryList.constBegin(); it != entryList.constEnd(); ++it) {
                idList << (*it)->id();
                alternativesItemModel->append(*it);
            }
            filterModel->setVisibleIds(idList);
        }
    }
};

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
    part->replaceXMLFile(KStandardDirs::locate("appdata", "findduplicatesui.rc"), KStandardDirs::locateLocal("appdata", "findduplicatesui.rc"), true);
}

void FindDuplicatesUI::slotFindDuplicates()
{
    FindDuplicates fd;
    QList<QList<Entry*> > cliques =  fd.findDuplicateEntries(d->editor->bibTeXModel()->bibTeXFile());

    KDialog dlg(d->part->widget());
    MergeWidget mw(d->editor->bibTeXModel()->bibTeXFile(), cliques, &dlg);
    dlg.setMainWidget(&mw);

    if (dlg.exec() == QDialog::Accepted) {
        // TODO
    }
}
