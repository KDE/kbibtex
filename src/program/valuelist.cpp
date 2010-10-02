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

#include <KComboBox>
#include <KDebug>

#include <QListView>
#include <QGridLayout>
#include <QStringListModel>

#include <bibtexfields.h>
#include <entry.h>
#include <mdiwidget.h>
#include "valuelist.h"

class ValueListModel: public QStringListModel
{
private:
    QStringList values;
    const File *file;
    const QString fName;

public:
    ValueListModel(const File *bibtexFile, const QString &fieldName, QObject *parent)
            : QStringListModel(parent), file(bibtexFile), fName(fieldName.toLower()) {
        updateValues();
    }

    void updateValues() {
        values.clear();
        for (File::ConstIterator fit = file->constBegin(); fit != file->constEnd(); ++fit) {
            const Entry *entry = dynamic_cast<const Entry*>(*fit);
            if (entry != NULL) {
                for (Entry::ConstIterator eit = entry->constBegin(); eit != entry->constEnd(); ++eit) {
                    QString key = eit.key().toLower();
                    if (key == fName) {
                        insertValue(eit.value());
                        break;
                    }
                }
            }
        }
        values.sort();
        setStringList(values);
    }

    void insertValue(const Value &value) {
        for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
            // TODO
        }
        const QString ptv = PlainTextValue::text(value, file);
        if (!values.contains(ptv))
            values << ptv;
    }
};

class ValueList::ValueListPrivate
{
private:
    MDIWidget *mdi;
    ValueList *p;
    KComboBox *comboboxFieldNames;
    QListView *listviewFieldValues;
    const QString fieldName;

public:
    const Entry *entry;
    const File *file;

    ValueListPrivate(MDIWidget *mdiWidget, ValueList *parent)
            : mdi(mdiWidget), p(parent) {
        setupGUI();
        initialize();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);

        comboboxFieldNames = new KComboBox(true, p);
        layout->addWidget(comboboxFieldNames, 0, 0, 1, 1);

        listviewFieldValues = new QListView(p);
        layout->addWidget(listviewFieldValues, 1, 0, 1, 1);

        p->setEnabled(false);

        connect(comboboxFieldNames, SIGNAL(activated(int)), p, SLOT(comboboxChanged()));
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

ValueList::ValueList(MDIWidget *mdiWidget, QWidget *parent)
        : QWidget(parent), d(new ValueListPrivate(mdiWidget, this))
{
    // TODO
}

void ValueList::setElement(Element* element, const File *file)
{
    d->entry = dynamic_cast<const Entry*>(element);
    if (d->entry != NULL && file != NULL && file != d->file) {
        d->file = file;
        d->update();
        setEnabled(true);
    } else
        setEnabled(false);

}

void ValueList::comboboxChanged()
{
    d->comboboxChanged();
}
