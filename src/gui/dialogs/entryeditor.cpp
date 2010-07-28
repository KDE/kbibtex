/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#include <QBuffer>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>

#include <KDebug>
#include <KComboBox>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>

#include <bibtexentries.h>
#include <bibtexfields.h>
#include <entry.h>
#include <entrylayout.h>
#include <fieldinput.h>
#include <fileexporterbibtex.h>
#include <fileimporterbibtex.h>
#include "entryeditor.h"

class EntryEditorSource : public QWidget
{
private:
    Entry *entry;
    QTextEdit *sourceEdit;

    void createGUI() {
        QGridLayout *layout = new QGridLayout(this);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);

        sourceEdit = new QTextEdit(this);
        layout->addWidget(sourceEdit, 0, 0, 1, 2);
        sourceEdit->document()->setDefaultFont(KGlobalSettings::fixedFont());

        KPushButton *buttonRestore = new KPushButton(i18n("Restore"), this);
        layout->addWidget(buttonRestore, 1, 1, 1, 1);
        connect(buttonRestore, SIGNAL(clicked()), parent(), SLOT(reset()));
        connect(sourceEdit->document(), SIGNAL(modificationChanged(bool)), this, SIGNAL(modified(bool)));
    }

public:
    EntryEditorSource(QWidget *parent)
            : QWidget(parent) {
        createGUI();
    }

    bool apply(Entry *entry) {
        bool result = false;
        QString text = sourceEdit->document()->toPlainText();
        QByteArray ba = text.toAscii();
        QBuffer buffer(&ba);
        buffer.open(QIODevice::ReadOnly);
        FileImporterBibTeX importer;
        File *file = importer.load(&buffer);
        buffer.close();
        if (file == NULL) return false;

        if (file->count() == 1) {
            Entry *readEntry = dynamic_cast<Entry*>(file->first());
            if (readEntry != NULL) {
                entry->operator =(*readEntry);
                result = true;
            }
        }

        delete file;
        return result;
    }

    void reset(const Entry *entry) {
        FileExporterBibTeX exporter;
        QBuffer textBuffer;
        textBuffer.open(QIODevice::WriteOnly);
        exporter.save(&textBuffer, entry, NULL);
        textBuffer.close();
        textBuffer.open(QIODevice::ReadOnly);
        QTextStream ts(&textBuffer);
        QString text = ts.readAll();
        sourceEdit->document()->setPlainText(text);
    }

    void apply() {
        apply(entry);
    }

    void reset() {
        reset(entry);
    }

    void setReadOnly(bool isReadOnly) {
        Q_UNUSED(isReadOnly);
        // TODO
    }

signals:
    void modified(bool enableApply);
};

class EntryEditorGUI : public QWidget
{
private:
    Entry *entry;
    KComboBox *entryType;
    KLineEdit *entryId;
    KLineEdit *otherFieldsName;
    FieldInput *otherFieldsContent;
    QListWidget *otherFieldsList;
    QMap<QString, FieldInput*> bibtexKeyToWidget;

    void createGUI() {
        QBoxLayout *layout = new QVBoxLayout(this);
        QWidget *refGui = createReferenceGUI(this);
        layout->addWidget(refGui, 0);
        QTabWidget *entriesGui = creatEntryLayoutGUI(this);
        layout->addWidget(entriesGui, 1);
        QWidget *otherGui = createOtherFieldsGUI(entriesGui);
        entriesGui->addTab(otherGui, i18n("Other"));
    }

    QWidget *createReferenceGUI(QWidget *parent) {
        QWidget *container = new QWidget(parent);
        QHBoxLayout *layout = new QHBoxLayout(container);

        QLabel *label = new QLabel(i18n("Type:"), container);
        layout->addWidget(label, 0);
        entryType = new KComboBox(container);
        entryType->setEditable(true);
        layout->addWidget(entryType, 1);
        label->setBuddy(entryType);

        layout->addSpacing(16);

        label = new QLabel(i18n("Id:"), container);
        layout->addWidget(label, 0);
        entryId = new KLineEdit(container);
        layout->addWidget(entryId, 1);
        label->setBuddy(entryId);

        BibTeXEntries *be = BibTeXEntries::self();
        for (BibTeXEntries::ConstIterator it = be->constBegin(); it != be->constEnd(); ++it)
            entryType->addItem(it->label, it->upperCamelCase);

        return container;
    }

    QTabWidget *creatEntryLayoutGUI(QWidget *parent) {
        QTabWidget *tabWidget = new QTabWidget(parent);

        BibTeXFields *bf = BibTeXFields::self();
        EntryLayout *el = EntryLayout::self();

        for (EntryLayout::ConstIterator elit = el->constBegin(); elit != el->constEnd(); ++elit) {
            EntryTabLayout etl = *elit;
            QWidget *container = new QWidget(tabWidget);
            tabWidget->addTab(container, etl.uiCaption);

            QGridLayout *layout = new QGridLayout(container);

            int mod = etl.singleFieldLayouts.size() / etl.columns;
            if (etl.singleFieldLayouts.size() % etl.columns > 0)
                ++mod;
            layout->setRowStretch(mod, 1);

            int col = 0, row = 0;
            for (QList<SingleFieldLayout>::ConstIterator sflit = etl.singleFieldLayouts.constBegin(); sflit != etl.singleFieldLayouts.constEnd(); ++sflit) {
                QLabel *label = new QLabel((*sflit).uiLabel + ":", container);
                layout->addWidget(label, row, col, 1, 1);
                label->setAlignment(Qt::AlignTop | Qt::AlignRight);

                const FieldDescription *fd = bf->find((*sflit).bibtexLabel);
                KBibTeX::TypeFlags typeFlags = fd == NULL ? KBibTeX::tfSource : fd->typeFlags;
                FieldInput *fieldInput = new FieldInput((*sflit).fieldInputLayout, typeFlags, container);
                layout->addWidget(fieldInput, row, col + 1, 1, 1);
                layout->setColumnStretch(col, 0);
                if ((*sflit).fieldInputLayout == KBibTeX::MultiLine || (*sflit).fieldInputLayout == KBibTeX::List)
                    layout->setRowStretch(row, 100);
                else {
                    layout->setRowStretch(row, 0);
                    layout->setAlignment(fieldInput, Qt::AlignTop);
                }
                layout->setColumnStretch(col + 1, 2);
                label->setBuddy(fieldInput);

                bibtexKeyToWidget.insert((*sflit).bibtexLabel, fieldInput);

                ++row;
                if (row >= mod) {
                    col += 2;
                    row = 0;
                }
            }
        }

        return tabWidget;
    }

    QWidget *createOtherFieldsGUI(QWidget *parent) {
        QWidget *container = new QWidget(parent);
        QGridLayout *layout = new QGridLayout(container);
        layout->setColumnStretch(0, 0);
        layout->setColumnStretch(1, 1);
        layout->setColumnStretch(2, 0);
        layout->setRowStretch(0, 0);
        layout->setRowStretch(1, 1);
        layout->setRowStretch(2, 0);
        layout->setRowStretch(3, 0);
        layout->setRowStretch(4, 1);

        QLabel *label = new QLabel(i18n("Name:"), container);
        layout->addWidget(label, 0, 0, 1, 1);
        otherFieldsName = new KLineEdit(container);
        layout->addWidget(otherFieldsName, 0, 1, 1, 1);
        label->setBuddy(otherFieldsName);

        KPushButton *buttonAddApply = new KPushButton(i18n("Add/Apply"), container);
        layout->addWidget(buttonAddApply, 0, 2, 1, 1);

        label = new QLabel(i18n("Content:"), container);
        layout->addWidget(label, 1, 0, 1, 1);
        otherFieldsContent = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, container);
        layout->addWidget(otherFieldsContent, 1, 1, 1, 2);
        label->setBuddy(otherFieldsContent);

        label = new QLabel(i18n("List:"), container);
        layout->addWidget(label, 2, 0, 3, 1);
        otherFieldsList = new QListWidget(container);
        layout->addWidget(otherFieldsList, 2, 1, 3, 1);
        label->setBuddy(otherFieldsList);
        KPushButton *buttonDelete = new KPushButton(i18n("Delete"), container);
        layout->addWidget(buttonDelete, 2, 2, 1, 1);
        KPushButton *buttonOpen = new KPushButton(i18n("Open"), container);
        layout->addWidget(buttonOpen, 3, 2, 1, 1);

        return container;
    }

public:
    EntryEditorGUI(QWidget *parent)
            : QWidget(parent) {
        createGUI();
    }

    void apply(Entry *entry) {
        entry->clear();

        BibTeXEntries *be = BibTeXEntries::self();
        QVariant var = entryType->itemData(entryType->currentIndex());
        QString type = var.toString();
        if (entryType->lineEdit()->isModified())
            type = be->format(entryType->lineEdit()->text(), KBibTeX::cUpperCamelCase);
        entry->setType(type);
        entry->setId(entryId->text());

        for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it) {
            Value value;
            it.value()->applyTo(value);
            if (!value.isEmpty()) {
                entry->insert(it.key(), value);
            }
        }

        // TODO: other fields
    }

    void reset(const Entry *entry) {
        entryId->setText(entry->id());
        BibTeXEntries *be = BibTeXEntries::self();
        QString type = be->format(entry->type(), KBibTeX::cUpperCamelCase);
        entryType->lineEdit()->setText(type);
        type = type.toLower();
        for (BibTeXEntries::ConstIterator it = be->constBegin(); it != be->constEnd(); ++it)
            if (type == it->upperCamelCase.toLower()) {
                entryType->lineEdit()->setText(it->label);
                break;
            }

        for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it)
            it.value()->clear();

        for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
            const QString key = it.key().toLower();
            if (bibtexKeyToWidget.contains(key)) {
                FieldInput *fieldInput = bibtexKeyToWidget[key];
                fieldInput->setValue(it.value());
            }
        }

    }

    void apply() {
        apply(entry);
    }

    void reset() {
        reset(entry);
    }

    void setReadOnly(bool isReadOnly) {
        entryId->setReadOnly(isReadOnly);
        entryType->lineEdit()->setReadOnly(isReadOnly);
        for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it)
            it.value()->setReadOnly(isReadOnly);
    }

signals:
    void modified(bool enableApply);
};

class EntryEditor::EntryEditorPrivate
{
private:
    EntryEditorGUI *tabGlobal;
    EntryEditorSource *tabSource;

public:
    Entry *entry;
    EntryEditor *p;
    bool isModified;

    EntryEditorPrivate(Entry *e, EntryEditor *parent)
            : entry(e), p(parent) {
        isModified = false;
    }

    void createGUI() {
        tabGlobal = new EntryEditorGUI(p);
        p->addTab(tabGlobal, i18n("Global"));
        tabSource = new EntryEditorSource(p);
        p->addTab(tabSource, i18n("Source"));
    }

    void apply() {
        apply(entry);
    }

    void apply(Entry *entry) {
        if (p->currentWidget() == tabGlobal)
            tabGlobal->apply(entry);
        else  if (p->currentWidget() == tabSource)
            tabSource->apply(entry);
    }

    void reset() {
        reset(entry);
    }

    void reset(const Entry *entry) {
        tabGlobal->reset(entry);
        tabSource->reset(entry);
    }

    void setReadOnly(bool isReadOnly) {
        tabGlobal->setReadOnly(isReadOnly);
        tabSource->setReadOnly(isReadOnly);
    }

    void switchTo(QWidget *newTab) {
        if (newTab == tabGlobal) {
            Entry temp;
            tabSource->apply(&temp);
            tabGlobal->reset(&temp);
        } else if (newTab == tabSource) {
            Entry temp;
            tabGlobal->apply(&temp);
            tabSource->reset(&temp);
        }
    }
};

EntryEditor::EntryEditor(Entry *entry, QWidget *parent)
        : QTabWidget(parent), d(new EntryEditorPrivate(entry, this))
{
    d->createGUI();
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
}

EntryEditor::EntryEditor(const Entry *entry, QWidget *parent)
        : QTabWidget(parent)
{
    Entry *e = new Entry(*entry);
    d = new EntryEditorPrivate(e, this);
    d->createGUI();
    setReadOnly(true);
}

void EntryEditor::apply()
{
    d->apply();
}

void EntryEditor::reset()
{
    d->reset();
    d->isModified = false;
    emit modified(false);
}

void EntryEditor::setReadOnly(bool isReadOnly)
{
    d->setReadOnly(isReadOnly);
}

void EntryEditor::tabChanged()
{
    d->switchTo(currentWidget());
}
