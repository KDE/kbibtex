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
#include <QDropEvent>

#include <KPushButton>
#include <KGlobalSettings>
#include <KLocale>
#include <KLineEdit>
#include <KComboBox>
#include <KSharedConfig>
#include <KConfigGroup>

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
#include <fieldlineedit.h>
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

const QString ElementWidget::keyElementWidgetLayout = QLatin1String("ElementWidgetLayout");
const Qt::Orientation ElementWidget::defaultElementWidgetLayout = Qt::Horizontal;


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
    for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it) {
        it.value()->clear();
        it.value()->setFile(m_file);
    }

    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        const QString key = it.key().toLower();
        if (bibtexKeyToWidget.contains(key)) {
            FieldInput *fieldInput = bibtexKeyToWidget[key];
            fieldInput->setElement(element);
            fieldInput->reset(it.value());
        }
    }

    return true;
}

void EntryConfiguredWidget::enableReqOptWidgets(const Element *element, bool forceEnable)
{
    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry == NULL) return;

    QStringList visibleItems;
    BibTeXEntries *be = BibTeXEntries::self();
    const BibTeXFields *bf = BibTeXFields::self();
    QString type = be->format(entry->type(), KBibTeX::cUpperCamelCase);
    type = type.toLower();
    for (BibTeXEntries::ConstIterator bit = be->constBegin(); bit != be->constEnd(); ++bit) {
        if (type == bit->upperCamelCase.toLower() || type == bit->upperCamelCaseAlt.toLower()) {
            /// this ugly conversion is necessary because we have a "^" (xor) and "|" (and/or)
            /// syntax to differentiate required items (not used yet, but will be used
            /// later if missing required items are marked).
            QString visible = bit->requiredItems.join(",");
            visible += "," + bit->optionalItems.join(",");
            visible = visible.replace("|", ",").replace("^", ",");
            visibleItems = visible.split(",");
            break;
        }
    }
    for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it) {
        const QString key = it.key().toLower();
        const FieldDescription &fd = bf->find(key);
        bool typeIndependent = fd.isNull() ? false : fd.typeIndependent;
        Value value;
        it.value()->apply(value);
        /// Hide non-required and non-optional type-dependent fields,
        /// except if the field has content
        bool isEnabled = forceEnable || typeIndependent || visibleItems.contains(key) || !value.isEmpty();
        it.value()->setEnabled(isEnabled);
        if (buddyList[it.value()])
            buddyList[it.value()]->setEnabled(isEnabled);
    }
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

void EntryConfiguredWidget::setFile(const File *file)
{
    if (file != NULL)
        for (QMap<QString, FieldInput*>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it) {
            /// list of unique values for same field
            QStringList list = file->uniqueEntryValuesList(it.key());
            /// for crossref fields, add all entries' ids
            if (it.key().toLower() == Entry::ftCrossRef)
                list.append(file->allKeys(File::etEntry));
            /// add macro keys
            list.append(file->allKeys(File::etMacro));

            it.value()->setCompletionItems(list);
        }

    ElementWidget::setFile(file);
}

bool EntryConfiguredWidget::canEdit(const Element *element)
{
    return typeid(*element) == typeid(Entry);
}

void EntryConfiguredWidget::createGUI()
{
    /// retrieve information from settings if labels should be
    /// above widgets or on the left side
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    const QString configGroupName(QLatin1String("User Interface"));
    KConfigGroup configGroup(config, configGroupName);
    Qt::Orientation layoutOrientation = (Qt::Orientation)configGroup.readEntry(keyElementWidgetLayout, (int)defaultElementWidgetLayout);

    QGridLayout *gridLayout = new QGridLayout(this);
    const BibTeXFields *bf = BibTeXFields::self();

    /// determine how many rows a column should have
    int mod = etl.singleFieldLayouts.size() / etl.columns;
    if (etl.singleFieldLayouts.size() % etl.columns > 0)
        ++mod;
    if (layoutOrientation == Qt::Vertical) mod *= 2;

    int row = 0, col = 0;
    foreach(const SingleFieldLayout &sfl, etl.singleFieldLayouts) {
        /// add extra space between "columns" of labels and widgets
        if (row == 0 && col > 1)
            gridLayout->setColumnMinimumWidth(col - 1, interColumnSpace);

        /// create an editing widget for this field
        const FieldDescription &fd = bf->find(sfl.bibtexLabel);
        KBibTeX::TypeFlags typeFlags = fd.isNull() ? KBibTeX::tfSource : fd.typeFlags;
        KBibTeX::TypeFlag preferredTypeFlag = fd.isNull() ? KBibTeX::tfSource : fd.preferredTypeFlag;
        FieldInput *fieldInput = new FieldInput(sfl.fieldInputLayout, preferredTypeFlag, typeFlags, this);
        fieldInput->setFieldKey(sfl.bibtexLabel);
        bibtexKeyToWidget.insert(sfl.bibtexLabel, fieldInput);
        connect(fieldInput, SIGNAL(modified()), this, SLOT(gotModified()));

        /// create a label next to the editing widget
        QLabel *label = new QLabel(QString("%1:").arg(sfl.uiLabel), this);
        label->setBuddy(fieldInput);
        buddyList.insert(fieldInput, label);

        /// position both label and editing widget according to set layout
        bool isVerticallyMinimumExpaning = sfl.fieldInputLayout == KBibTeX::MultiLine || sfl.fieldInputLayout == KBibTeX::List || sfl.fieldInputLayout == KBibTeX::PersonList || sfl.fieldInputLayout == KBibTeX::KeywordList;
        gridLayout->addWidget(label, row, col, 1, 1, (isVerticallyMinimumExpaning ? Qt::AlignTop : Qt::AlignVCenter) | (layoutOrientation == Qt::Horizontal ? Qt::AlignRight : Qt::AlignLeft));
        if (layoutOrientation == Qt::Horizontal) ++col;
        else ++row;
        gridLayout->addWidget(fieldInput, row, col, 1, 1);
        gridLayout->setRowStretch(row, isVerticallyMinimumExpaning ? 1000 : 0);

        /// move position counter
        ++row;
        if (layoutOrientation == Qt::Horizontal) --col;

        /// check if column is full
        if (row >= mod) {
            /// set columns' stretch
            if (layoutOrientation == Qt::Horizontal) {
                gridLayout->setColumnStretch(col, 1);
                gridLayout->setColumnStretch(col + 1, 1000);
            } else
                gridLayout->setColumnStretch(col, 1000);
            /// update/reset position in layout
            row = 0;
            col += layoutOrientation == Qt::Horizontal ? 3 : 2;
        }
    }

    /// fill tab's bottom with space
    gridLayout->setRowStretch(mod, 1);

    /// set last column's stretch
    if (row > 0) {
        if (layoutOrientation == Qt::Horizontal) {
            gridLayout->setColumnStretch(col, 1);
            gridLayout->setColumnStretch(col + 1, 1000);
        } else
            gridLayout->setColumnStretch(col, 1000);
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
    connect(entryType, SIGNAL(editTextChanged(QString)), this, SIGNAL(entryTypeChanged()));
}


FilesWidget::FilesWidget(QWidget *parent)
        : ElementWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    fileList = new FieldInput(KBibTeX::UrlList, KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, this);
    fileList->setFieldKey(QLatin1String("^external"));
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
    fileList->setElement(element);
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

    fieldName->setReadOnly(isReadOnly);
    fieldContent->setReadOnly(isReadOnly);

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
    /// retrieve information from settings if labels should be
    /// above widgets or on the left side
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    const QString configGroupName(QLatin1String("User Interface"));
    KConfigGroup configGroup(config, configGroupName);
    Qt::Orientation layoutOrientation = (Qt::Orientation)configGroup.readEntry(keyElementWidgetLayout, (int)defaultElementWidgetLayout);

    QGridLayout *layout = new QGridLayout(this);
    /// set row and column stretches based on chosen layout
    layout->setColumnStretch(0, layoutOrientation == Qt::Horizontal ? 0 : 1);
    layout->setColumnStretch(1, layoutOrientation == Qt::Horizontal ? 1 : 0);
    layout->setColumnStretch(2, 0);
    layout->setRowStretch(0, 0);
    layout->setRowStretch(layoutOrientation == Qt::Horizontal ? 1 : 3, 1);
    layout->setRowStretch(2, 0);
    layout->setRowStretch(layoutOrientation == Qt::Horizontal ? 3 : 6, 0);
    layout->setRowStretch(layoutOrientation == Qt::Horizontal ? 4 : 7, 1);

    QLabel *label = new QLabel(i18n("Name:"), this);
    layout->addWidget(label, layoutOrientation == Qt::Horizontal ? 0 : 0,  layoutOrientation == Qt::Horizontal ? 0 : 0, 1, 1, (layoutOrientation == Qt::Horizontal ? Qt::AlignRight : Qt::AlignLeft));

    fieldName = new KLineEdit(this);
    layout->addWidget(fieldName, layoutOrientation == Qt::Horizontal ? 0 : 1, layoutOrientation == Qt::Horizontal ? 1 : 0, 1, 1);
    label->setBuddy(fieldName);

    buttonAddApply = new KPushButton(KIcon("list-add"), i18n("Add"), this);
    buttonAddApply->setEnabled(false);
    layout->addWidget(buttonAddApply, layoutOrientation == Qt::Horizontal ? 0 : 1, layoutOrientation == Qt::Horizontal ? 2 : 1, 1, 1);

    label = new QLabel(i18n("Content:"), this);
    layout->addWidget(label, layoutOrientation == Qt::Horizontal ? 1 : 2, layoutOrientation == Qt::Horizontal ? 0 : 0, 1, 1, (layoutOrientation == Qt::Horizontal ? Qt::AlignRight : Qt::AlignLeft));
    fieldContent = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, KBibTeX::tfSource, this);
    layout->addWidget(fieldContent, layoutOrientation == Qt::Horizontal ? 1 : 3, layoutOrientation == Qt::Horizontal ? 1 : 0, 1, 2);
    label->setBuddy(fieldContent);

    label = new QLabel(i18n("List:"), this);
    layout->addWidget(label, layoutOrientation == Qt::Horizontal ? 2 : 4, layoutOrientation == Qt::Horizontal ? 0 : 0, 1, 1, (layoutOrientation == Qt::Horizontal ? Qt::AlignRight : Qt::AlignLeft));

    otherFieldsList = new QTreeWidget(this);
    otherFieldsList->setHeaderLabels(QStringList() << i18n("Key") << i18n("Value"));
    layout->addWidget(otherFieldsList, layoutOrientation == Qt::Horizontal ? 2 : 5, layoutOrientation == Qt::Horizontal ? 1 : 0, 3, 1);
    label->setBuddy(otherFieldsList);

    buttonDelete = new KPushButton(KIcon("list-remove"), i18n("Delete"), this);
    buttonDelete->setEnabled(false);
    layout->addWidget(buttonDelete, layoutOrientation == Qt::Horizontal ? 2 : 5, layoutOrientation == Qt::Horizontal ? 2 : 1, 1, 1);
    buttonOpen = new KPushButton(KIcon("document-open"), i18n("Open"), this);
    buttonOpen->setEnabled(false);
    layout->addWidget(buttonOpen, layoutOrientation == Qt::Horizontal ? 3 : 6, layoutOrientation == Qt::Horizontal ? 2 : 1, 1, 1);

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


class SourceWidget::SourceWidgetTextEdit : public QTextEdit
{
public:
    SourceWidgetTextEdit(QWidget *parent)
            : QTextEdit(parent) {
        // nothing
    }

protected:
    virtual void dropEvent(QDropEvent *event) {
        FileImporterBibTeX importer;
        FileExporterBibTeX exporter;
        const File *file = importer.fromString(event->mimeData()->text());
        if (file->count() == 1)
            document()->setPlainText(exporter.toString(file->first()));
        else
            QTextEdit::dropEvent(event);
    }
};

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
            } else {
                Preamble *preamble = dynamic_cast<Preamble*>(element);
                Preamble *readPreamble = dynamic_cast<Preamble*>(file->first());
                if (readPreamble != NULL && preamble != NULL) {
                    preamble->operator =(*readPreamble);
                    result = true;
                }
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
    exporter.setEncoding(QLatin1String("utf-8"));
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
    layout->setRowStretch(0, 1);
    layout->setRowStretch(1, 0);

    sourceEdit = new SourceWidgetTextEdit(this);
    layout->addWidget(sourceEdit, 0, 0, 1, 3);
    sourceEdit->document()->setDefaultFont(KGlobalSettings::fixedFont());
    sourceEdit->setTabStopWidth(QFontMetrics(sourceEdit->font()).averageCharWidth() * 4);

    m_buttonRestore = new KPushButton(KIcon("edit-undo"), i18n("Restore"), this);
    layout->addWidget(m_buttonRestore, 1, 1, 1, 1);
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
