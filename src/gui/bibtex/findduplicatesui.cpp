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

#include <QWidget>
#include <QBoxLayout>
#include <QLabel>
#include <QSortFilterProxyModel>

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

        editor = new BibTeXEditor(this);
        layout->addWidget(editor);
        editor->setReadOnly(true);

        model = new CheckableBibTeXFileModel(this);
        model->setBibTeXFile(new File(*file));
        filterModel = new FilterIdBibTeXFileModel(this);
        filterModel->setSourceModel(model);
        editor->setModel(filterModel);

        switchClique(0);
    }

    void switchClique(int i) {
        if (i >= 0 && i < cliques.size()) {
            QList<Entry*> entryList = cliques[i];
            QStringList idList;
            for (QList<Entry*>::ConstIterator it = entryList.constBegin(); it != entryList.constEnd(); ++it)
                idList << (*it)->id();
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
