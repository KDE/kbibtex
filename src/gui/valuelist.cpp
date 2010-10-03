/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
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

#include <typeinfo>

#include <KComboBox>
#include <KDebug>

#include <QListView>
#include <QGridLayout>
#include <QStringListModel>

#include <bibtexfields.h>
#include <entry.h>
#include "valuelist.h"

class ValueListModel: public QStringListModel
{
private:
    const File *file;
    const QString fName;

public:
    ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent)
            : QStringListModel(parent), file(bibtexFile), fName(fieldName.toLower()) {
        updateValues();
    }

    void updateValues() {
        QStringList valueList;

        for (File::ConstIterator fit = file->constBegin(); fit != file->constEnd(); ++fit) {
            const Entry *entry = dynamic_cast<const Entry*>(*fit);
            if (entry != NULL) {
                for (Entry::ConstIterator eit = entry->constBegin(); eit != entry->constEnd(); ++eit) {
                    QString key = eit.key().toLower();
                    if (key == fName) {
                        insertValue(eit.value(), valueList);
                        break;
                    }
                }
            }
        }
        valueList.sort();
        setStringList(valueList);
    }

    void insertValue(const Value &value, QStringList &valueList) {
        for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
            if (typeid(*(*it)) == typeid(Person)) {
                const Person *person = static_cast<const Person*>(*it);
                QString text = person->lastName();
                if (!person->firstName().isEmpty()) text.append(", " + person->firstName());
                // TODO: other parts of name
                if (!valueList.contains(text))
                    valueList << text;
            } else {
                const QString text = PlainTextValue::text(*(*it), file);
                if (!valueList.contains(text))
                    valueList << text;
            }
        }
    }
};

class ValueList::ValueListPrivate
{
private:
    ValueList *p;
    QListView *listviewFieldValues;
    const QString fieldName;

public:
    const File *file;
    KComboBox *comboboxFieldNames;

    ValueListPrivate(ValueList *parent)
            : p(parent) {
        setupGUI();
        initialize();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        comboboxFieldNames = new KComboBox(true, p);
        layout->addWidget(comboboxFieldNames, 0, 0, 1, 1);

        listviewFieldValues = new QListView(p);
        layout->addWidget(listviewFieldValues, 1, 0, 1, 1);
        listviewFieldValues->setEditTriggers(QAbstractItemView::NoEditTriggers);

        p->setEnabled(false);

        connect(comboboxFieldNames, SIGNAL(activated(int)), p, SLOT(comboboxChanged()));
        connect(listviewFieldValues, SIGNAL(activated(const QModelIndex &)), p, SLOT(listItemActivated(const QModelIndex &)));
    }

    void initialize() {
        const BibTeXFields *bibtexFields = BibTeXFields::self();

        comboboxFieldNames->clear();
        for (BibTeXFields::ConstIterator it = bibtexFields->constBegin(); it != bibtexFields->constEnd(); ++it) {
            FieldDescription fd = *it;
            if (!fd.upperCamelCaseAlt.isEmpty()) continue; /// keep only "single" fields and not combined ones like "Author or Editor"
            comboboxFieldNames->addItem(fd.label, fd.upperCamelCase);
        }
    }

    void comboboxChanged() {
        update();
    }

    void update() {
        QVariant var = comboboxFieldNames->itemData(comboboxFieldNames->currentIndex());
        QString text = var.toString();
        if (text.isEmpty()) text = comboboxFieldNames->currentText();

        listviewFieldValues->setModel(new ValueListModel(file, text, listviewFieldValues));
    }
};

ValueList::ValueList(QWidget *parent)
        : QWidget(parent), d(new ValueListPrivate(this))
{
    // TODO
}

void ValueList::setElement(Element*, const File *file)
{
    if (file != NULL && file != d->file) {
        d->file = file;
        d->update();
        setEnabled(true);
    }
}

void ValueList::comboboxChanged()
{
    d->comboboxChanged();
}

void ValueList::listItemActivated(const QModelIndex &index)
{
    QString itemText = index.data(Qt::DisplayRole).toString();
    QVariant fieldVar = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex());
    QString fieldText = fieldVar.toString();
    if (fieldText.isEmpty()) fieldText = d->comboboxFieldNames->currentText();

    SortFilterBibTeXFileModel::FilterQuery fq;
    fq.terms << itemText;
    fq.combination = SortFilterBibTeXFileModel::EveryTerm;
    fq.field = fieldText;

    emit filterChanged(fq);
}
