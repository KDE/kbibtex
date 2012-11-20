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

#include <typeinfo>

#include <QLayout>
#include <QTextEdit>
#include <QBuffer>
#include <QLabel>
#include <QTreeWidget>
#include <QFileInfo>
#include <QDropEvent>
#include <QMenu>
#include <QStyle>

#include <KPushButton>
#include <KGlobalSettings>
#include <KLocale>
#include <KLineEdit>
#include <KComboBox>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KMimeType>
#include <KRun>

#include "idsuggestions.h"
#include "fileinfo.h"
#include <kbibtexnamespace.h>
#include "bibtexentries.h"
#include "bibtexfields.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"
#include <fieldinput.h>
#include "entry.h"
#include "macro.h"
#include "preamble.h"
#include <fieldlineedit.h>
#include "elementwidgets.h"

static const unsigned int interColumnSpace = 16;
static const QStringList keyStart = QStringList() << Entry::ftUrl << QLatin1String("postscript") << Entry::ftLocalFile << Entry::ftDOI << Entry::ftFile << QLatin1String("ee") << QLatin1String("biburl");
static const char *PropertyIdSuggestion = "PropertyIdSuggestion";

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


EntryConfiguredWidget::EntryConfiguredWidget(QSharedPointer<EntryTabLayout> &entryTabLayout, QWidget *parent)
        : ElementWidget(parent), etl(entryTabLayout)
{
    gridLayout = new QGridLayout(this);
    createGUI();
}

EntryConfiguredWidget::~EntryConfiguredWidget()
{
    delete[] listOfLabeledFieldInput;
}

bool EntryConfiguredWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (entry.isNull()) return false;

    for (QMap<QString, FieldInput *>::ConstIterator it = bibtexKeyToWidget.constBegin(); it != bibtexKeyToWidget.constEnd(); ++it) {
        Value value;
        it.value()->apply(value);
        entry->remove(it.key());
        if (!value.isEmpty())
            entry->insert(it.key(), value);
    }

    return true;
}

bool EntryConfiguredWidget::reset(QSharedPointer<const Element> element)
{
    QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (entry.isNull()) return false;

    /// clear all widgets
    for (QMap<QString, FieldInput *>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it) {
        it.value()->clear();
        it.value()->setFile(m_file);
    }

    for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
        const QString key = it.key().toLower();
        if (bibtexKeyToWidget.contains(key)) {
            FieldInput *fieldInput = bibtexKeyToWidget[key];
            fieldInput->setElement(element.data());
            fieldInput->reset(it.value());
        }
    }

    return true;
}

void EntryConfiguredWidget::showReqOptWidgets(bool forceVisible, const QString &entryType)
{
    layoutGUI(forceVisible, entryType);
}

void EntryConfiguredWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    for (QMap<QString, FieldInput *>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it)
        it.value()->setReadOnly(isReadOnly);
}

QString EntryConfiguredWidget::label()
{
    return etl->uiCaption;
}

KIcon EntryConfiguredWidget::icon()
{
    return KIcon(etl->iconName);
}

void EntryConfiguredWidget::setFile(const File *file)
{
    if (file != NULL)
        for (QMap<QString, FieldInput *>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it) {
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
    const BibTeXFields *bf = BibTeXFields::self();

    /// store information on number of widgets and columns in class variable
    fieldInputCount = etl->singleFieldLayouts.size();
    numCols = etl->columns;
    /// initialize list of field input widgets plus labels
    listOfLabeledFieldInput = new LabeledFieldInput*[fieldInputCount];

    int i = 0;
    foreach(const SingleFieldLayout &sfl, etl->singleFieldLayouts) {
        LabeledFieldInput *labeledFieldInput = new LabeledFieldInput;

        /// create an editing widget for this field
        const FieldDescription *fd = bf->find(sfl.bibtexLabel);
        KBibTeX::TypeFlags typeFlags = fd == NULL ? KBibTeX::tfSource : fd->typeFlags;
        KBibTeX::TypeFlag preferredTypeFlag = fd == NULL ? KBibTeX::tfSource : fd->preferredTypeFlag;
        labeledFieldInput->fieldInput = new FieldInput(sfl.fieldInputLayout, preferredTypeFlag, typeFlags, this);
        labeledFieldInput->fieldInput->setFieldKey(sfl.bibtexLabel);
        bibtexKeyToWidget.insert(sfl.bibtexLabel, labeledFieldInput->fieldInput);
        connect(labeledFieldInput->fieldInput, SIGNAL(modified()), this, SLOT(gotModified()));

        /// memorize if field input should grow vertically (e.g. is a list)
        labeledFieldInput->isVerticallyMinimumExpaning = sfl.fieldInputLayout == KBibTeX::MultiLine || sfl.fieldInputLayout == KBibTeX::List || sfl.fieldInputLayout == KBibTeX::PersonList || sfl.fieldInputLayout == KBibTeX::KeywordList;

        /// create a label next to the editing widget
        labeledFieldInput->label = new QLabel(QString("%1:").arg(sfl.uiLabel), this);
        labeledFieldInput->label->setBuddy(labeledFieldInput->fieldInput);
        /// align label's text vertically to match field input
        Qt::Alignment horizontalAlignment = (Qt::Alignment)(labeledFieldInput->label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment) & 0x001f);
        labeledFieldInput->label->setAlignment(horizontalAlignment | (labeledFieldInput->isVerticallyMinimumExpaning ? Qt::AlignTop : Qt::AlignVCenter));

        listOfLabeledFieldInput[i] = labeledFieldInput;

        ++i;
    }

    layoutGUI(true);
}

void EntryConfiguredWidget::layoutGUI(bool forceVisible, const QString &entryType)
{
    const BibTeXFields *bf = BibTeXFields::self();

    QStringList visibleItems;
    if (!forceVisible && !entryType.isEmpty()) {
        const QString entryTypeLc = entryType.toLower();
        BibTeXEntries *be = BibTeXEntries::self();
        for (BibTeXEntries::ConstIterator bit = be->constBegin(); bit != be->constEnd(); ++bit) {
            if (entryTypeLc == bit->upperCamelCase.toLower() || entryTypeLc == bit->upperCamelCaseAlt.toLower()) {
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
    } else if (!forceVisible)
        kDebug() << "EntryType should not be empty if visibility is not enforced";

    /// variables to keep track which and how many field inputs will be visible
    int countVisible = 0;
    bool *visible = new bool[fieldInputCount];
    /// ... and if any field input is vertically expaning
    /// (e.g. a list, important for layout)
    bool anyoneVerticallyExpanding = false;

    for (int i = fieldInputCount - 1; i >= 0; --i) {
        listOfLabeledFieldInput[i]->label->setVisible(false);
        listOfLabeledFieldInput[i]->fieldInput->setVisible(false);
        gridLayout->removeWidget(listOfLabeledFieldInput[i]->label);
        gridLayout->removeWidget(listOfLabeledFieldInput[i]->fieldInput);

        const QString key = bibtexKeyToWidget.key(listOfLabeledFieldInput[i]->fieldInput).toLower();
        const FieldDescription *fd = bf->find(key);
        bool typeIndependent = fd == NULL ? false : fd->typeIndependent;
        Value value;
        listOfLabeledFieldInput[i]->fieldInput->apply(value);
        /// Hide non-required and non-optional type-dependent fields,
        /// except if the field has content
        visible[i] = forceVisible || typeIndependent || !value.isEmpty() || visibleItems.contains(key);

        if (visible[i]) {
            ++countVisible;
            anyoneVerticallyExpanding |= listOfLabeledFieldInput[i]->isVerticallyMinimumExpaning;
        }
    }

    int numRows = countVisible / numCols;
    if (countVisible % numCols > 0)
        ++numRows;
    gridLayout->setRowStretch(numRows, anyoneVerticallyExpanding ? 0 : 1000);

    int col = 0, row = 0;
    for (int i = 0; i < fieldInputCount; ++i)
        if (visible[i]) {
            /// add label and field input to new position in grid layout
            gridLayout->addWidget(listOfLabeledFieldInput[i]->label, row, col * 3);
            gridLayout->addWidget(listOfLabeledFieldInput[i]->fieldInput, row, col * 3 + 1);

            /// set row stretch
            gridLayout->setRowStretch(row, listOfLabeledFieldInput[i]->isVerticallyMinimumExpaning ? 1000 : 0);
            /// set column stretch and spacing
            gridLayout->setColumnStretch(col * 3, 1);
            gridLayout->setColumnStretch(col * 3 + 1, 1000);
            if (col > 0)
                gridLayout->setColumnMinimumWidth(col * 3 - 1, interColumnSpace);

            /// count rows and columns correctly
            ++row;
            if (row >= numRows) {
                row = 0;
                ++col;
            }

            /// finally, set label and field input visible again
            listOfLabeledFieldInput[i]->label->setVisible(true);
            listOfLabeledFieldInput[i]->fieldInput->setVisible(true); // FIXME expensive!
        }

    if (countVisible > 0) {
        /// fix row stretch
        for (int i = numRows + 1; i < 100; ++i)
            gridLayout->setRowStretch(i, 0);
        /// hide unused columns
        for (int i = (col + (row == 0 ? 0 : 1)) * 3 - 1; i < 100; ++i) {
            gridLayout->setColumnMinimumWidth(i, 0);
            gridLayout->setColumnStretch(i, 0);
        }
    }

    delete[] visible;
}

ReferenceWidget::ReferenceWidget(QWidget *parent)
        : ElementWidget(parent), m_applyElement(NULL)
{
    createGUI();
}

bool ReferenceWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    bool result = false;
    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (!entry.isNull()) {
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
        QSharedPointer<Macro> macro = element.dynamicCast<Macro>();
        if (!macro.isNull()) {
            macro->setKey(entryId->text());
            result = true;
        }
    }

    return result;
}

bool ReferenceWidget::reset(QSharedPointer<const Element> element)
{
    /// if signals are not deactivated, the "modified" signal would be emitted when
    /// resetting the widgets' values
    disconnect(entryType, SIGNAL(editTextChanged(QString)), this, SLOT(gotModified()));
    disconnect(entryId, SIGNAL(textChanged(QString)), this, SLOT(gotModified()));

    bool result = false;
    QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (!entry.isNull()) {
        entryType->setEnabled(!isReadOnly);
        buttonSuggestId->setEnabled(!isReadOnly);
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
        buttonSuggestId->setEnabled(false);
        QSharedPointer<const Macro> macro = element.dynamicCast<const Macro>();
        if (!macro.isNull()) {
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
    entryType->setEnabled(!isReadOnly);
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
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    layout->addWidget(label);
    layout->addWidget(entryType);

    layout->addSpacing(interColumnSpace);

    entryId = new KLineEdit(this);
    entryId->setClearButtonShown(true);
    label = new QLabel(i18n("Id:"), this);
    label->setBuddy(entryId);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    layout->addWidget(label);
    layout->addWidget(entryId);

    BibTeXEntries *be = BibTeXEntries::self();
    for (BibTeXEntries::ConstIterator it = be->constBegin(); it != be->constEnd(); ++it)
        entryType->addItem(it->label, it->upperCamelCase);

    /// Button with a menu listing a set of preconfigured id suggestions
    buttonSuggestId = new KPushButton(KIcon("view-filter"), QLatin1String(""), this);
    buttonSuggestId->setToolTip(i18n("Select a suggested id for this entry"));
    layout->addWidget(buttonSuggestId);
    QMenu *suggestionsMenu = new QMenu(buttonSuggestId);
    buttonSuggestId->setMenu(suggestionsMenu);

    connect(entryType->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(gotModified()));
    connect(entryId, SIGNAL(textEdited(QString)), this, SLOT(gotModified()));
    connect(entryType->lineEdit(), SIGNAL(textEdited(QString)), this, SIGNAL(entryTypeChanged()));
    connect(entryType, SIGNAL(currentIndexChanged(int)), this, SIGNAL(entryTypeChanged()));
    connect(suggestionsMenu, SIGNAL(aboutToShow()), this, SLOT(prepareSuggestionsMenu()));
}

void ReferenceWidget::prepareSuggestionsMenu()
{
    /// Collect information on the current entry as it is edited
    QSharedPointer<Entry> internalEntry(new Entry());
    m_applyElement->apply(internalEntry);

    static const IdSuggestions *idSuggestions = new IdSuggestions();
    QMenu *suggestionsMenu = buttonSuggestId->menu();
    suggestionsMenu->clear();

    /// Keep track of shown suggestions to avoid duplicates
    QSet<QString> knownIdSuggestion;
    QString defaultSuggestion = idSuggestions->defaultFormatId(*internalEntry.data());

    foreach(QString suggestion, idSuggestions->formatIdList(*internalEntry.data())) {
        bool isDefault = suggestion == defaultSuggestion;

        /// Test for duplicate ids, use fallback ids with numeric suffix
        if (m_file != NULL && m_file->containsKey(suggestion)) {
            int suffix = 2;
            const QString suggestionBase = suggestion;
            while (m_file->containsKey(suggestion = suggestionBase + QChar('_') + QString::number(suffix)))
                ++suffix;
        }

        /// Keep track of shown suggestions to avoid duplicates
        if (knownIdSuggestion.contains(suggestion)) continue;
        else knownIdSuggestion.insert(suggestion);

        /// Create action for suggestion, use icon depending if default or not
        QAction *suggestionAction = new QAction(suggestion, suggestionsMenu);
        suggestionAction->setIcon(KIcon(isDefault ? "favorites" : "view-filter"));

        /// Mesh action into GUI
        suggestionsMenu->addAction(suggestionAction);
        connect(suggestionAction, SIGNAL(triggered()), this, SLOT(insertSuggestionFromAction()));
        /// Remember suggestion string for time when action gets triggered
        suggestionAction->setProperty(PropertyIdSuggestion, suggestion);
    }
}

void ReferenceWidget::insertSuggestionFromAction()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    if (action != NULL) {
        const QString suggestion = action->property(PropertyIdSuggestion).toString();
        entryId->setText(suggestion);
    }
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

bool FilesWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (entry.isNull()) return false;

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
        const QSharedPointer<VerbatimText> verbatimText = (*it).dynamicCast<VerbatimText>();
        if (!verbatimText.isNull()) {
            QString text = verbatimText->text();
            if (KBibTeX::urlRegExp.indexIn(text) > -1) {
                /// add full URL
                VerbatimText *newVT = new VerbatimText(KBibTeX::urlRegExp.cap(0));
                /// test for duplicates
                if (urlValue.contains(*newVT))
                    delete newVT;
                else
                    urlValue.append(QSharedPointer<VerbatimText>(newVT));
            } else if (KBibTeX::doiRegExp.indexIn(text) > -1) {
                /// add DOI
                VerbatimText *newVT = new VerbatimText(KBibTeX::doiRegExp.cap(0));
                /// test for duplicates
                if (doiValue.contains(*newVT))
                    delete newVT;
                else
                    doiValue.append(QSharedPointer<VerbatimText>(newVT));
            } else {
                /// add anything else (e.g. local file)
                VerbatimText *newVT = new VerbatimText(*verbatimText);
                /// test for duplicates
                if (localFileValue.contains(*newVT))
                    delete newVT;
                else
                    localFileValue.append(QSharedPointer<VerbatimText>(newVT));
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

bool FilesWidget::reset(QSharedPointer<const Element> element)
{
    QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (entry.isNull()) return false;

    Value combinedValue;
    for (QStringList::ConstIterator it = keyStart.constBegin(); it != keyStart.constEnd(); ++it)
        for (int i = 1; i < 32; ++i) {  /// FIXME replace number by constant
            QString key = *it;
            if (i > 1) key.append(QString::number(i));
            const Value &value = entry->operator [](key);
            for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it)
                combinedValue.append(*it);
        }
    fileList->setElement(element.data());
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
    internalEntry = QSharedPointer<Entry>(new Entry());
    createGUI();
}

bool OtherFieldsWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (entry.isNull()) return false;

    for (QStringList::ConstIterator it = deletedKeys.constBegin(); it != deletedKeys.constEnd(); ++it)
        entry->remove(*it);
    for (QStringList::ConstIterator it = modifiedKeys.constBegin(); it != modifiedKeys.constEnd(); ++it) {
        entry->remove(*it);
        entry->insert(*it, internalEntry->value(*it));
    }

    return true;
}

bool OtherFieldsWidget::reset(QSharedPointer<const Element> element)
{
    QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (entry.isNull()) return false;

    internalEntry = QSharedPointer<Entry>(new Entry(*entry.data()));
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

    Q_ASSERT_X(otherFieldsList->currentItem() != NULL, "OtherFieldsWidget::actionDelete", "otherFieldsList->currentItem() is NULL");
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
    if (currentUrl.isValid()) {
        /// Guess mime type for url to open
        KMimeType::Ptr mimeType = FileInfo::mimeTypeForUrl(currentUrl);
        QString mimeTypeName = mimeType->name();
        if (mimeTypeName == QLatin1String("application/octet-stream"))
            mimeTypeName = QLatin1String("text/html");
        /// Ask KDE subsystem to open url in viewer matching mime type
        KRun::runUrl(currentUrl, mimeTypeName, this, false, false);
    }
}

void OtherFieldsWidget::createGUI()
{
    /// retrieve information from settings if labels should be
    /// above widgets or on the left side
    KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
    const QString configGroupName(QLatin1String("User Interface"));
    KConfigGroup configGroup(config, configGroupName);

    QGridLayout *layout = new QGridLayout(this);
    /// set row and column stretches based on chosen layout
    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(2, 0);
    layout->setRowStretch(0, 0);
    layout->setRowStretch(1, 1);
    layout->setRowStretch(2, 0);
    layout->setRowStretch(3, 0);
    layout->setRowStretch(4, 1);

    QLabel *label = new QLabel(i18n("Name:"), this);
    layout->addWidget(label, 0, 0, 1, Qt::AlignRight);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));

    fieldName = new KLineEdit(this);
    layout->addWidget(fieldName, 0, 1, 1, 1);
    label->setBuddy(fieldName);

    buttonAddApply = new KPushButton(KIcon("list-add"), i18n("Add"), this);
    buttonAddApply->setEnabled(false);
    layout->addWidget(buttonAddApply, 0, 2, 1, 1);

    label = new QLabel(i18n("Content:"), this);
    layout->addWidget(label, 1, 0, 1, 1, Qt::AlignRight);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    fieldContent = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, KBibTeX::tfSource, this);
    layout->addWidget(fieldContent, 1, 1, 1, 2);
    label->setBuddy(fieldContent);

    label = new QLabel(i18n("List:"), this);
    layout->addWidget(label, 2,  0, 1, 1,  Qt::AlignRight);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));

    otherFieldsList = new QTreeWidget(this);
    otherFieldsList->setHeaderLabels(QStringList() << i18n("Key") << i18n("Value"));
    layout->addWidget(otherFieldsList, 2, 1, 3, 1);
    label->setBuddy(otherFieldsList);

    buttonDelete = new KPushButton(KIcon("list-remove"), i18n("Delete"), this);
    buttonDelete->setEnabled(false);
    layout->addWidget(buttonDelete,  2, 2, 1, 1);
    buttonOpen = new KPushButton(KIcon("document-open"), i18n("Open"), this);
    buttonOpen->setEnabled(false);
    layout->addWidget(buttonOpen, 3, 2, 1, 1);

    connect(otherFieldsList, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(listElementExecuted(QTreeWidgetItem *, int)));
    connect(otherFieldsList, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(listCurrentChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
    connect(otherFieldsList, SIGNAL(itemSelectionChanged()), this, SLOT(updateGUI()));
    connect(fieldName, SIGNAL(textEdited(QString)), this, SLOT(updateGUI()));
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
        buttonAddApply->setIcon(internalEntry->contains(key) ? KIcon("edit") : KIcon("list-add"));
    }
}

MacroWidget::MacroWidget(QWidget *parent)
        : ElementWidget(parent)
{
    createGUI();
}

bool MacroWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QSharedPointer<Macro> macro = element.dynamicCast<Macro>();
    if (macro.isNull()) return false;

    Value value;
    bool result = fieldInputValue->apply(value);
    macro->setValue(value);

    return result;
}

bool MacroWidget::reset(QSharedPointer<const Element> element)
{
    QSharedPointer<const Macro> macro = element.dynamicCast<const Macro>();
    if (macro.isNull()) return false;

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
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
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

bool PreambleWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QSharedPointer<Preamble> preamble = element.dynamicCast<Preamble>();
    if (preamble.isNull()) return false;

    Value value;
    bool result = fieldInputValue->apply(value);
    preamble->setValue(value);

    return result;
}

bool PreambleWidget::reset(QSharedPointer<const Element> element)
{
    QSharedPointer<const Preamble> preamble = element.dynamicCast<const Preamble>();
    if (preamble.isNull()) return false;

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
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
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

bool SourceWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; ///< never save data if in read-only mode

    const QString text = sourceEdit->document()->toPlainText();
    FileImporterBibTeX importer;
    File *file = importer.fromString(text);
    if (file == NULL) return false;

    bool result = false;
    if (file->count() == 1) {
        QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        QSharedPointer<Entry> readEntry = file->first().dynamicCast<Entry>();
        if (!readEntry.isNull() && !entry.isNull()) {
            entry->operator =(*readEntry.data()); //entry = readEntry;
            result = true;
        } else {
            QSharedPointer<Macro> macro = element.dynamicCast<Macro>();
            QSharedPointer<Macro> readMacro = file->first().dynamicCast<Macro>();
            if (!readMacro.isNull() && !macro.isNull()) {
                macro->operator =(*readMacro.data());
                result = true;
            } else {
                QSharedPointer<Preamble> preamble = element.dynamicCast<Preamble>();
                QSharedPointer<Preamble> readPreamble = file->first().dynamicCast<Preamble>();
                if (!readPreamble.isNull() && !preamble.isNull()) {
                    preamble->operator =(*readPreamble.data());
                    result = true;
                } else
                    kWarning() << "Do not know how to apply source code";
            }
        }
    } else
        kDebug() << "Expected exactly 1 BibTeX element in source code, but found" << file->count() << "instead";

    delete file;
    return result;
}

bool SourceWidget::reset(QSharedPointer<const Element> element)
{
    /// if signals are not deactivated, the "modified" signal would be emitted when
    /// resetting the widget's value
    disconnect(sourceEdit, SIGNAL(textChanged()), this, SLOT(gotModified()));

    FileExporterBibTeX exporter;
    exporter.setEncoding(QLatin1String("utf-8"));
    const QString exportedText = exporter.toString(element);
    if (!exportedText.isNull()) {
        originalText = exportedText;
        sourceEdit->document()->setPlainText(originalText);
    }

    connect(sourceEdit, SIGNAL(textChanged()), this, SLOT(gotModified()));

    return !exportedText.isNull();
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
