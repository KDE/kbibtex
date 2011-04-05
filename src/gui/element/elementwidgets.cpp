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

#include <QLayout>
#include <QTextEdit>
#include <QBuffer>
#include <QLabel>
#include <QTreeWidget>
#include <QFileInfo>
#include <QDesktopServices>

#include <KPushButton>
#include <KGlobalSettings>
#include <KLocale>
#include <KLineEdit>
#include <KComboBox>
#include <KDebug>

#include <kbibtexnamespace.h>
#include <bibtexentries.h>
#include <bibtexfields.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <file.h>
#include <fieldinput.h>
#include <entry.h>
#include <macro.h>
#include <preamble.h>
#include "elementwidgets.h"

static const unsigned int interColumnSpace = 16;
static const QStringList keyStart = QStringList() << Entry::ftUrl << QLatin1String("postscript") << Entry::ftLocalFile << Entry::ftDOI << QLatin1String("ee") << QLatin1String("biburl");

ElementWidget::ElementWidget(QWidget *parent): QWidget(parent), isReadOnly(false), m_isModified(false)
{
    // nothing
};

bool ElementWidget::isModified() const
{
    return m_isModified;
}

void ElementWidget::setModified(bool newIsModified)
{
    m_isModified = newIsModified;

    emit modified(newIsModified);
}

void ElementWidget::gotModified()
{
    setModified(true);
}

EntryConfiguredWidget::EntryConfiguredWidget(EntryTabLayout &entryTabLayout, QWidget *parent)
        : ElementWidget(parent), etl(entryTabLayout)
{
    createGUI();
}

bool EntryConfiguredWidget::apply(Element *element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    Entry *entry = dynamic_cast<Entry*>(element);
    if (entry == NULL) return false;

    for (QMap<QString, FieldInput*>::ConstIterator it = bibtexKeyToWidget.constBegin(); it != bibtexKeyToWidget.constEnd(); ++it) {
        Value value;
        it.value()->apply(value);
        entry->remove(it.key());
        if (!value.isEmpty())
            entry->insert(it.key(), value);
    }

    return true;
}

bool EntryConfiguredWidget::reset(const Element *element)
{
    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry == NULL) return false;

    /// clear all widgets
    for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it)
        it.value()->clear();

    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        const QString key = it.key().toLower();
        if (bibtexKeyToWidget.contains(key)) {
            FieldInput *fieldInput = bibtexKeyToWidget[key];
            fieldInput->setFile(m_file);
            fieldInput->reset(it.value());
        }
    }

    return true;
}

void EntryConfiguredWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it)
        it.value()->setReadOnly(isReadOnly);
}

QString EntryConfiguredWidget::label()
{
    return etl.uiCaption;
}

KIcon EntryConfiguredWidget::icon()
{
    return KIcon(etl.iconName);
}

bool EntryConfiguredWidget::canEdit(const Element *element)
{
    return typeid(*element) == typeid(Entry);
}

void EntryConfiguredWidget::createGUI()
{
    QGridLayout *gridLayout = new QGridLayout(this);
    BibTeXFields *bf = BibTeXFields::self();

    int mod = etl.singleFieldLayouts.size() / etl.columns;
    if (etl.singleFieldLayouts.size() % etl.columns > 0)
        ++mod;

    int row = 0, col = 0;
    for (QList<SingleFieldLayout>::ConstIterator sflit = etl.singleFieldLayouts.constBegin(); sflit != etl.singleFieldLayouts.constEnd(); ++sflit) {
        if (row == 0 && col > 1)
            gridLayout->setColumnMinimumWidth(col - 1, interColumnSpace);

        const FieldDescription *fd = bf->find((*sflit).bibtexLabel);
        KBibTeX::TypeFlags typeFlags = fd == NULL ? KBibTeX::tfSource : fd->typeFlags;
        KBibTeX::TypeFlag preferredTypeFlag = fd == NULL ? KBibTeX::tfSource : fd->preferredTypeFlag;
        FieldInput *fieldInput = new FieldInput((*sflit).fieldInputLayout, preferredTypeFlag, typeFlags, this);
        bibtexKeyToWidget.insert((*sflit).bibtexLabel, fieldInput);
        connect(fieldInput, SIGNAL(modified()), this, SLOT(gotModified()));

        bool isVerticallyMinimumExpaning = (*sflit).fieldInputLayout == KBibTeX::MultiLine || (*sflit).fieldInputLayout == KBibTeX::List || (*sflit).fieldInputLayout == KBibTeX::PersonList;

        QLabel *label = new QLabel((*sflit).uiLabel + ":", this);
        label->setBuddy(fieldInput);
        gridLayout->addWidget(label, row, col, 1, 1, (isVerticallyMinimumExpaning ? Qt::AlignTop : Qt::AlignVCenter) | Qt::AlignRight);
        gridLayout->addWidget(fieldInput, row, col + 1, 1, 1);

        gridLayout->setRowStretch(row, isVerticallyMinimumExpaning ? 1000 : 0);

        ++row;
        if (row >= mod) {
            gridLayout->setColumnStretch(col, 1);
            gridLayout->setColumnStretch(col + 1, 1000);
            row = 0;
            col += 3;
        }
    }

    gridLayout->setRowStretch(mod, 1);
    if (row > 0) {
        gridLayout->setColumnStretch(col, 1);
        gridLayout->setColumnStretch(col + 1, 1000);
    }
}


ReferenceWidget::ReferenceWidget(QWidget *parent)
        : ElementWidget(parent)
{
    createGUI();
}

bool ReferenceWidget::apply(Element *element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    bool result = false;
    Entry *entry = dynamic_cast<Entry*>(element);
    if (entry != NULL) {
        BibTeXEntries *be = BibTeXEntries::self();
        QString type = QString::null;
        if (entryType->currentIndex() < 0 || entryType->lineEdit()->isModified())
            type = be->format(entryType->lineEdit()->text(), KBibTeX::cUpperCamelCase);
        else
            type = entryType->itemData(entryType->currentIndex()).toString();
        entry->setType(type);

        entry->setId(entryId->text());
        result = true;
    } else {
        Macro *macro = dynamic_cast<Macro*>(element);
        if (macro != NULL) {
            macro->setKey(entryId->text());
            result = true;
        }
    }

    return result;
}

bool ReferenceWidget::reset(const Element *element)
{
    /// if signals are not deactivated, the "modified" signal would be emitted when
    /// resetting the widgets' values
    disconnect(entryType, SIGNAL(editTextChanged(QString)), this, SLOT(gotModified()));
    disconnect(entryId, SIGNAL(textChanged(QString)), this, SLOT(gotModified()));

    bool result = false;
    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL) {
        entryType->setEnabled(true);
        BibTeXEntries *be = BibTeXEntries::self();
        QString type = be->format(entry->type(), KBibTeX::cUpperCamelCase);
        entryType->setCurrentIndex(-1);
        entryType->lineEdit()->setText(type);
        type = type.toLower();
        int index = 0;
        for (BibTeXEntries::ConstIterator it = be->constBegin(); it != be->constEnd(); ++it, ++index)
            if (type == it->upperCamelCase.toLower() || type == it->upperCamelCaseAlt.toLower()) {
                entryType->setCurrentIndex(index);
                break;
            }
        entryId->setText(entry->id());
        result = true;
    } else {
        entryType->setEnabled(false);
        const   Macro *macro = dynamic_cast<const Macro*>(element);
        if (macro != NULL) {
            entryType->lineEdit()->setText(i18n("Macro"));
            entryId->setText(macro->key());
            result = true;
        }
    }

    connect(entryType, SIGNAL(editTextChanged(QString)), this, SLOT(gotModified()));
    connect(entryId, SIGNAL(textChanged(QString)), this, SLOT(gotModified()));

    return result;
}

void ReferenceWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    entryId->setReadOnly(isReadOnly);
    entryType->lineEdit()->setReadOnly(isReadOnly);
}

QString ReferenceWidget::label()
{
    return QString::null;
}

KIcon ReferenceWidget::icon()
{
    return KIcon();
}

bool ReferenceWidget::canEdit(const Element *element)
{
    return typeid(*element) == typeid(Entry) || typeid(*element) == typeid(Macro);
}

void ReferenceWidget::createGUI()
{
    QHBoxLayout *layout = new QHBoxLayout(this);

    entryType = new KComboBox(this);
    entryType->setEditable(true);
    entryType->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    QLabel *label = new QLabel(i18n("Type:"), this);
    label->setBuddy(entryType);
    layout->addWidget(label);
    layout->addWidget(entryType);

    layout->addSpacing(interColumnSpace);

    entryId = new KLineEdit(this);
    entryId->setClearButtonShown(true);
    label = new QLabel(i18n("Id:"), this);
    label->setBuddy(entryId);
    layout->addWidget(label);
    layout->addWidget(entryId);

    BibTeXEntries *be = BibTeXEntries::self();
    for (BibTeXEntries::ConstIterator it = be->constBegin(); it != be->constEnd(); ++it)
        entryType->addItem(it->label, it->upperCamelCase);

    connect(entryType, SIGNAL(editTextChanged(QString)), this, SLOT(gotModified()));
    connect(entryId, SIGNAL(textChanged(QString)), this, SLOT(gotModified()));
}


FilesWidget::FilesWidget(QWidget *parent)
        : ElementWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    fileList = new FieldInput(KBibTeX::UrlList, KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, this);
    layout->addWidget(fileList);
    connect(fileList, SIGNAL(modified()), this, SLOT(gotModified()));
}

bool FilesWidget::apply(Element *element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    Entry* entry = dynamic_cast<Entry*>(element);
    if (entry == NULL) return false;

    for (QStringList::ConstIterator it = keyStart.constBegin(); it != keyStart.constEnd(); ++it)
        for (int i = 1; i < 32; ++i) {  /// FIXME replace number by constant
            QString key = *it;
            if (i > 1) key.append(QString::number(i));
            entry->remove(key);
        }

    Value combinedValue;
    fileList->apply(combinedValue);

    Value urlValue, doiValue, localFileValue;

    for (Value::ConstIterator it = combinedValue.constBegin(); it != combinedValue.constEnd(); ++it) {
        const VerbatimText *verbatimText = dynamic_cast<const VerbatimText *>(*it);
        if (verbatimText != NULL) {
            QString text = verbatimText->text();
            if (KBibTeX::urlRegExp.indexIn(text) > -1) {
                /// add full URL
                VerbatimText *newVT = new VerbatimText(KBibTeX::urlRegExp.cap(0));
                /// test for duplicates
                if (urlValue.contains(*newVT))
                    delete newVT;
                else
                    urlValue.append(newVT);
            } else if (KBibTeX::doiRegExp.indexIn(text) > -1) {
                /// add DOI
                VerbatimText *newVT = new VerbatimText(KBibTeX::doiRegExp.cap(0));
                /// test for duplicates
                if (doiValue.contains(*newVT))
                    delete newVT;
                else
                    doiValue.append(newVT);
            } else {
                /// add anything else (e.g. local file)
                VerbatimText *newVT = new VerbatimText(*verbatimText);
                /// test for duplicates
                if (localFileValue.contains(*newVT))
                    delete newVT;
                else
                    localFileValue.append(newVT);
            }
        }
    }

    if (urlValue.isEmpty())
        entry->remove(Entry::ftUrl);
    else
        entry->insert(Entry::ftUrl, urlValue);

    if (localFileValue.isEmpty())
        entry->remove(Entry::ftLocalFile);
    else
        entry->insert(Entry::ftLocalFile, localFileValue);

    if (doiValue.isEmpty())
        entry->remove(Entry::ftDOI);
    else
        entry->insert(Entry::ftDOI, doiValue);

    return true;
}

bool FilesWidget::reset(const Element *element)
{
    const Entry* entry = dynamic_cast<const Entry*>(element);
    if (entry == NULL) return false;

    Value combinedValue;
    for (QStringList::ConstIterator it = keyStart.constBegin(); it != keyStart.constEnd(); ++it)
        for (int i = 1; i < 32; ++i) {  /// FIXME replace number by constant
            QString key = *it;
            if (i > 1) key.append(QString::number(i));
            const Value &value = entry->operator [](key);
            for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it)
                combinedValue.append(*it);
        }
    fileList->setFile(m_file);
    fileList->reset(combinedValue);

    return true;
}

void FilesWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);
    fileList->setReadOnly(isReadOnly);
}

QString FilesWidget::label()
{
    return i18n("External");
}

KIcon FilesWidget::icon()
{
    return KIcon("emblem-symbolic-link");
}

bool FilesWidget::canEdit(const Element *element)
{
    return typeid(*element) == typeid(Entry);
}


OtherFieldsWidget::OtherFieldsWidget(const QStringList &blacklistedFields, QWidget *parent)
        : ElementWidget(parent), blackListed(blacklistedFields)
{
    internalEntry = new Entry();
    createGUI();
}

OtherFieldsWidget::~OtherFieldsWidget()
{
    delete internalEntry;
}

bool OtherFieldsWidget::apply(Element *element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    Entry* entry = dynamic_cast<Entry*>(element);
    if (entry == NULL) return false;

    for (QStringList::ConstIterator it = deletedKeys.constBegin(); it != deletedKeys.constEnd(); ++it)
        entry->remove(*it);
    for (QStringList::ConstIterator it = modifiedKeys.constBegin(); it != modifiedKeys.constEnd(); ++it) {
        entry->remove(*it);
        entry->insert(*it, internalEntry->value(*it));
    }

    return true;
}

bool OtherFieldsWidget::reset(const Element *element)
{
    const Entry* entry = dynamic_cast<const Entry*>(element);
    if (entry == NULL) return false;

    internalEntry->operator =(*entry);
    deletedKeys.clear(); // FIXME clearing list may be premature here...
    modifiedKeys.clear(); // FIXME clearing list may be premature here...
    updateList();
    updateGUI();

    return true;
}

void OtherFieldsWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    /// will take care of enabled/disabling buttons
    updateGUI();
    updateList();
}

QString OtherFieldsWidget::label()
{
    return i18n("Other Fields");
}

KIcon OtherFieldsWidget::icon()
{
    return KIcon("other");
}

bool OtherFieldsWidget::canEdit(const Element *element)
{
    return typeid(*element) == typeid(Entry);
}

void OtherFieldsWidget::listElementExecuted(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column) /// we do not care which column got clicked
    QString key = item->text(0);
    fieldName->setText(key);
    fieldContent->reset(internalEntry->value(key));
}

void OtherFieldsWidget::listCurrentChanged(QTreeWidgetItem *item, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous)
    bool validUrl = false;
    bool somethingSelected = item != NULL;
    buttonDelete->setEnabled(somethingSelected && !isReadOnly);
    if (somethingSelected) {
        currentUrl = KUrl(item->text(1));
        validUrl = currentUrl.isValid() && currentUrl.isLocalFile() & QFileInfo(currentUrl.pathOrUrl()).exists();
        if (!validUrl) {
            if (KBibTeX::urlRegExp.indexIn(item->text(1)) > -1) {
                currentUrl = KUrl(KBibTeX::urlRegExp.cap(0));
                validUrl = currentUrl.isValid();
                buttonOpen->setEnabled(validUrl);
            }
        }
    }

    if (!validUrl)
        currentUrl = KUrl();
    buttonOpen->setEnabled(validUrl);
}

void OtherFieldsWidget::actionAddApply()
{
    if (isReadOnly) return; /// never modify anything if in read-only mode

    QString key = fieldName->text();
    Value value;
    if (!fieldContent->apply(value)) return;

    if (internalEntry->contains(key))
        internalEntry->remove(key);
    internalEntry->insert(key, value);

    if (!modifiedKeys.contains(key)) modifiedKeys << key;

    updateList();
    updateGUI();

    gotModified();
}

void OtherFieldsWidget::actionDelete()
{
    if (isReadOnly) return; /// never modify anything if in read-only mode

    Q_ASSERT(otherFieldsList->currentItem() != NULL);
    QString key = otherFieldsList->currentItem()->text(0);
    if (!deletedKeys.contains(key)) deletedKeys << key;

    internalEntry->remove(key);
    updateList();
    updateGUI();
    listCurrentChanged(otherFieldsList->currentItem(), NULL);

    gotModified();
}

void OtherFieldsWidget::actionOpen()
{
    if (currentUrl.isValid())
        QDesktopServices::openUrl(currentUrl); // TODO KDE way?
}

void OtherFieldsWidget::createGUI()
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(2, 0);
    layout->setRowStretch(0, 0);
    layout->setRowStretch(1, 1);
    layout->setRowStretch(2, 0);
    layout->setRowStretch(3, 0);
    layout->setRowStretch(4, 1);

    QLabel *label = new QLabel(i18n("Name:"), this);
    layout->addWidget(label, 0, 0, 1, 1);
    fieldName = new KLineEdit(this);
    layout->addWidget(fieldName, 0, 1, 1, 1);
    label->setBuddy(fieldName);

    buttonAddApply = new KPushButton(KIcon("list-add"), i18n("Add"), this);
    buttonAddApply->setEnabled(false);
    layout->addWidget(buttonAddApply, 0, 2, 1, 1);

    label = new QLabel(i18n("Content:"), this);
    layout->addWidget(label, 1, 0, 1, 1);
    fieldContent = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, KBibTeX::tfSource, this);
    layout->addWidget(fieldContent, 1, 1, 1, 2);
    label->setBuddy(fieldContent);

    label = new QLabel(i18n("List:"), this);
    layout->addWidget(label, 2, 0, 3, 1);
    otherFieldsList = new QTreeWidget(this);
    QStringList header;
    header << i18n("Key") << i18n("Value");
    otherFieldsList->setHeaderLabels(header);
    layout->addWidget(otherFieldsList, 2, 1, 3, 1);
    label->setBuddy(otherFieldsList);
    buttonDelete = new KPushButton(KIcon("list-remove"), i18n("Delete"), this);
    buttonDelete->setEnabled(false);
    layout->addWidget(buttonDelete, 2, 2, 1, 1);
    buttonOpen = new KPushButton(KIcon("document-open"), i18n("Open"), this);
    buttonOpen->setEnabled(false);
    layout->addWidget(buttonOpen, 3, 2, 1, 1);

    connect(otherFieldsList, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(listElementExecuted(QTreeWidgetItem*, int)));
    connect(otherFieldsList, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(listCurrentChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(otherFieldsList, SIGNAL(itemSelectionChanged()), this, SLOT(updateGUI()));
    connect(fieldName, SIGNAL(textChanged(QString)), this, SLOT(updateGUI()));
    connect(buttonAddApply, SIGNAL(clicked()), this, SLOT(actionAddApply()));
    connect(buttonDelete, SIGNAL(clicked()), this, SLOT(actionDelete()));
    connect(buttonOpen, SIGNAL(clicked()), this, SLOT(actionOpen()));
}

void OtherFieldsWidget::updateList()
{
    QString selText = otherFieldsList->selectedItems().isEmpty() ? QString::null : otherFieldsList->selectedItems().first()->text(0);
    QString curText = otherFieldsList->currentItem() == NULL ? QString::null : otherFieldsList->currentItem()->text(0);
    otherFieldsList->clear();

    for (Entry::ConstIterator it = internalEntry->constBegin(); it != internalEntry->constEnd(); ++it)
        if (!blackListed.contains(it.key().toLower())) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, it.key());
            item->setText(1, PlainTextValue::text(it.value()));
            item->setIcon(0, KIcon("entry")); // FIXME
            otherFieldsList->addTopLevelItem(item);
            item->setSelected(selText == it.key());
            if (it.key() == curText)
                otherFieldsList->setCurrentItem(item);
        }
}

void OtherFieldsWidget::updateGUI()
{
    QString key = fieldName->text();
    if (key.isEmpty() || blackListed.contains(key, Qt::CaseInsensitive)) // TODO check for more (e.g. spaces)
        buttonAddApply->setEnabled(false);
    else {
        buttonAddApply->setEnabled(!isReadOnly);
        buttonAddApply->setText(internalEntry->contains(key) ? i18n("Apply") : i18n("Add"));
        buttonAddApply->setIcon(internalEntry->contains(key) ? KIcon("edit") : KIcon("add"));
    }
}

MacroWidget::MacroWidget(QWidget *parent)
        : ElementWidget(parent)
{
    createGUI();
}

bool MacroWidget::apply(Element *element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    Macro* macro = dynamic_cast<Macro*>(element);
    if (macro == NULL) return false;

    Value value;
    bool result = fieldInputValue->apply(value);
    macro->setValue(value);

    return result;
}

bool MacroWidget::reset(const Element *element)
{
    const Macro* macro = dynamic_cast<const Macro*>(element);
    if (macro == NULL) return false;

    return fieldInputValue->reset(macro->value());
}

void MacroWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    fieldInputValue->setReadOnly(isReadOnly);
}

QString MacroWidget::label()
{
    return i18n("Macro");
}

KIcon MacroWidget::icon()
{
    return KIcon("macro");
}

bool MacroWidget::canEdit(const Element *element)
{
    return typeid(*element) == typeid(Macro);
}

void MacroWidget::createGUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel(i18n("Value:"), this);
    layout->addWidget(label, 0);
    fieldInputValue = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfSource, this);
    layout->addWidget(fieldInputValue, 1);
    label->setBuddy(fieldInputValue);

    connect(fieldInputValue, SIGNAL(modified()), this, SLOT(gotModified()));
}


PreambleWidget::PreambleWidget(QWidget *parent)
        : ElementWidget(parent)
{
    createGUI();
}

bool PreambleWidget::apply(Element *element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    Preamble* preamble = dynamic_cast<Preamble*>(element);
    if (preamble == NULL) return false;

    Value value;
    bool result = fieldInputValue->apply(value);
    preamble->setValue(value);

    return result;
}

bool PreambleWidget::reset(const Element *element)
{
    const Preamble* preamble = dynamic_cast<const Preamble*>(element);
    if (preamble == NULL) return false;

    return fieldInputValue->reset(preamble->value());
}

void PreambleWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    fieldInputValue->setReadOnly(isReadOnly);
}

QString PreambleWidget::label()
{
    return i18n("Preamble");
}

KIcon PreambleWidget::icon()
{
    return KIcon("preamble");
}

bool PreambleWidget::canEdit(const Element *element)
{
    return typeid(*element) == typeid(Preamble);
}

void PreambleWidget::createGUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel(i18n("Value:"), this);
    layout->addWidget(label, 0);
    fieldInputValue = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, KBibTeX::tfSource, this); // FIXME: other editing modes beyond Source applicable?
    layout->addWidget(fieldInputValue, 1);
    label->setBuddy(fieldInputValue);

    connect(fieldInputValue, SIGNAL(modified()), this, SLOT(gotModified()));
}


SourceWidget::SourceWidget(QWidget *parent)
        : ElementWidget(parent)
{
    createGUI();
}

bool SourceWidget::apply(Element *element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QString text = sourceEdit->document()->toPlainText();
    FileImporterBibTeX importer;
    File *file = importer.fromString(text);
    if (file == NULL) return false;

    bool result = false;
    if (file->count() == 1) {
        Entry *entry = dynamic_cast<Entry*>(element);
        Entry *readEntry = dynamic_cast<Entry*>(file->first());
        if (readEntry != NULL && entry != NULL) {
            entry->operator =(*readEntry);
            result = true;
        } else {
            Macro *macro = dynamic_cast<Macro*>(element);
            Macro *readMacro = dynamic_cast<Macro*>(file->first());
            if (readMacro != NULL && macro != NULL) {
                macro->operator =(*readMacro);
                result = true;
            }
        }
    }

    delete file;
    return result;
}

bool SourceWidget::reset(const Element *element)
{
    /// if signals are not deactivated, the "modified" signal would be emitted when
    /// resetting the widget's value
    disconnect(sourceEdit, SIGNAL(textChanged()), this, SLOT(gotModified()));

    FileExporterBibTeX exporter;
    QBuffer textBuffer;
    textBuffer.open(QIODevice::WriteOnly);
    bool result = exporter.save(&textBuffer, element, NULL);
    textBuffer.close();
    textBuffer.open(QIODevice::ReadOnly);
    QTextStream ts(&textBuffer);
    originalText = ts.readAll();
    sourceEdit->document()->setPlainText(originalText);

    connect(sourceEdit, SIGNAL(textChanged()), this, SLOT(gotModified()));

    return result;
}

void SourceWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    m_buttonRestore->setEnabled(!isReadOnly);
    sourceEdit->setReadOnly(isReadOnly);
}

QString SourceWidget::label()
{
    return i18n("Source");
}

KIcon SourceWidget::icon()
{
    return KIcon("code-context");
}

bool SourceWidget::canEdit(const Element *element)
{
    Q_UNUSED(element)
    return true; /// source widget should be able to edit any element
}

void SourceWidget::createGUI()
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 0);
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 0);

    sourceEdit = new QTextEdit(this);
    layout->addWidget(sourceEdit, 0, 0, 1, 3);
    sourceEdit->document()->setDefaultFont(KGlobalSettings::fixedFont());
    sourceEdit->setTabStopWidth(QFontMetrics(sourceEdit->font()).averageCharWidth() * 4);

    m_buttonRestore = new KPushButton(KIcon("edit-undo"), i18n("Restore"), this);
    layout->addWidget(m_buttonRestore, 1, 2, 1, 1);
    connect(m_buttonRestore, SIGNAL(clicked()), this, SLOT(reset()));

    connect(sourceEdit, SIGNAL(textChanged()), this, SLOT(gotModified()));
}

void SourceWidget::reset()
{
    /// if signals are not deactivated, the "modified" signal would be emitted when
    /// resetting the widget's value
    disconnect(sourceEdit, SIGNAL(textChanged()), this, SLOT(gotModified()));

    sourceEdit->document()->setPlainText(originalText);
    setModified(false);

    connect(sourceEdit, SIGNAL(textChanged()), this, SLOT(gotModified()));
}
