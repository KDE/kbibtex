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

#include "elementwidgets.h"

#include <QLayout>
#include <QBuffer>
#include <QLabel>
#include <QTreeWidget>
#include <QFileInfo>
#include <QDropEvent>
#include <QMenu>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMimeData>
#include <QSortFilterProxyModel>
#include <QStyle>
#include <QPushButton>
#include <QFontDatabase>

#include <KLocalizedString>
#include <KLineEdit>
#include <KComboBox>
#include <KRun>
#include <KTextEdit>
#include <kio_version.h>

#include "idsuggestions.h"
#include "fileinfo.h"
#include "kbibtex.h"
#include "bibtexentries.h"
#include "bibtexfields.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "file.h"
#include "fieldinput.h"
#include "entry.h"
#include "macro.h"
#include "preamble.h"
#include "fieldlineedit.h"
#include "logging_gui.h"

static const unsigned int interColumnSpace = 16;
static const char *PropertyIdSuggestion = "PropertyIdSuggestion";

ElementWidget::ElementWidget(QWidget *parent)
        : QWidget(parent), isReadOnly(false), m_file(nullptr), m_isModified(false)
{
    /// nothing
}

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


EntryConfiguredWidget::EntryConfiguredWidget(const QSharedPointer<const EntryTabLayout> &entryTabLayout, QWidget *parent)
        : ElementWidget(parent), fieldInputCount(entryTabLayout->singleFieldLayouts.size()), numCols(entryTabLayout->columns), etl(entryTabLayout)
{
    gridLayout = new QGridLayout(this);

    /// Initialize list of field input widgets plus labels
    listOfLabeledFieldInput = new LabeledFieldInput*[fieldInputCount];

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
        it.value()->setFile(m_file);
        it.value()->clear();
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

QIcon EntryConfiguredWidget::icon()
{
    return QIcon::fromTheme(etl->iconName);
}

void EntryConfiguredWidget::setFile(const File *file)
{
    for (QMap<QString, FieldInput *>::Iterator it = bibtexKeyToWidget.begin(); it != bibtexKeyToWidget.end(); ++it) {
        it.value()->setFile(file);
        if (file != nullptr) {
            /// list of unique values for same field
            QStringList list = file->uniqueEntryValuesList(it.key());
            /// for crossref fields, add all entries' ids
            if (it.key().toLower() == Entry::ftCrossRef)
                list.append(file->allKeys(File::etEntry));
            /// add macro keys
            list.append(file->allKeys(File::etMacro));

            it.value()->setCompletionItems(list);
        }
    }

    ElementWidget::setFile(file);
}

bool EntryConfiguredWidget::canEdit(const Element *element)
{
    return Entry::isEntry(*element);
}

void EntryConfiguredWidget::createGUI()
{
    const BibTeXFields *bf = BibTeXFields::self();

    int i = 0;
    for (const SingleFieldLayout &sfl : const_cast<const QList<SingleFieldLayout> &>(etl->singleFieldLayouts)) {
        LabeledFieldInput *labeledFieldInput = new LabeledFieldInput;

        /// create an editing widget for this field
        const FieldDescription &fd = bf->find(sfl.bibtexLabel);
        labeledFieldInput->fieldInput = new FieldInput(sfl.fieldInputLayout, fd.preferredTypeFlag, fd.typeFlags, this);
        labeledFieldInput->fieldInput->setFieldKey(sfl.bibtexLabel);
        bibtexKeyToWidget.insert(sfl.bibtexLabel, labeledFieldInput->fieldInput);
        connect(labeledFieldInput->fieldInput, &FieldInput::modified, this, &EntryConfiguredWidget::gotModified);

        /// memorize if field input should grow vertically (e.g. is a list)
        labeledFieldInput->isVerticallyMinimumExpaning = sfl.fieldInputLayout == KBibTeX::MultiLine || sfl.fieldInputLayout == KBibTeX::List || sfl.fieldInputLayout == KBibTeX::PersonList || sfl.fieldInputLayout == KBibTeX::KeywordList;

        /// create a label next to the editing widget
        labeledFieldInput->label = new QLabel(QString(QStringLiteral("%1:")).arg(sfl.uiLabel), this);
        labeledFieldInput->label->setBuddy(labeledFieldInput->fieldInput->buddy());
        /// align label's text vertically to match field input
        const Qt::Alignment horizontalAlignment = static_cast<Qt::Alignment>(labeledFieldInput->label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment)) & Qt::AlignHorizontal_Mask;
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
        const BibTeXEntries *be = BibTeXEntries::self();
        for (const auto &ed : const_cast<const BibTeXEntries &>(*be)) {
            if (entryTypeLc == ed.upperCamelCase.toLower() || entryTypeLc == ed.upperCamelCaseAlt.toLower()) {
                /// this ugly conversion is necessary because we have a "^" (xor) and "|" (and/or)
                /// syntax to differentiate required items (not used yet, but will be used
                /// later if missing required items are marked).
                QString visible = ed.requiredItems.join(QStringLiteral(","));
                visible += QLatin1Char(',') + ed.optionalItems.join(QStringLiteral(","));
                visible = visible.replace(QLatin1Char('|'), QLatin1Char(',')).replace(QLatin1Char('^'), QLatin1Char(','));
                visibleItems = visible.split(QStringLiteral(","));
                break;
            }
        }
    } else if (!forceVisible) {
        // TODO is this an error condition?
    }

    /// variables to keep track which and how many field inputs will be visible
    int countVisible = 0;
    QScopedArrayPointer<bool> visible(new bool[fieldInputCount]);
    /// ... and if any field input is vertically expaning
    /// (e.g. a list, important for layout)
    bool anyoneVerticallyExpanding = false;

    for (int i = fieldInputCount - 1; i >= 0; --i) {
        listOfLabeledFieldInput[i]->label->setVisible(false);
        listOfLabeledFieldInput[i]->fieldInput->setVisible(false);
        gridLayout->removeWidget(listOfLabeledFieldInput[i]->label);
        gridLayout->removeWidget(listOfLabeledFieldInput[i]->fieldInput);

        const QString key = bibtexKeyToWidget.key(listOfLabeledFieldInput[i]->fieldInput).toLower();
        const FieldDescription &fd = bf->find(key);
        Value value;
        listOfLabeledFieldInput[i]->fieldInput->apply(value);
        /// Hide non-required and non-optional type-dependent fields,
        /// except if the field has content
        visible[i] = forceVisible || fd.typeIndependent || !value.isEmpty() || visibleItems.contains(key);

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
}

ReferenceWidget::ReferenceWidget(QWidget *parent)
        : ElementWidget(parent), m_applyElement(nullptr), m_entryIdManuallySet(false), m_element(QSharedPointer<Element>())
{
    createGUI();
}

bool ReferenceWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    bool result = false;
    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (!entry.isNull()) {
        entry->setType(computeType());
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
    disconnect(entryType->lineEdit(), &KLineEdit::textChanged, this, &ReferenceWidget::gotModified);
    disconnect(entryId, &KLineEdit::textEdited, this, &ReferenceWidget::entryIdManuallyChanged);

    bool result = false;
    QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (!entry.isNull()) {
        entryType->setEnabled(!isReadOnly);
        buttonSuggestId->setEnabled(!isReadOnly);
        const BibTeXEntries *be = BibTeXEntries::self();
        QString type = be->format(entry->type(), KBibTeX::cUpperCamelCase);
        int index = entryType->findData(type);
        if (index == -1) {
            const QString typeLower(type.toLower());
            for (const auto &ed : const_cast<const BibTeXEntries &>(*be))
                if (typeLower == ed.upperCamelCaseAlt.toLower()) {
                    index = entryType->findData(ed.upperCamelCase);
                    break;
                }
        }
        entryType->setCurrentIndex(index);
        if (index == -1) {
            /// A customized value not known to KBibTeX
            entryType->lineEdit()->setText(type);
        }

        entryId->setText(entry->id());
        /// New entries have no values. Use this fact
        /// to recognize new entries, for which it is
        /// allowed to automatic set their ids
        /// if a default id suggestion had been specified.
        m_entryIdManuallySet = entry->count() > 0;

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

    connect(entryId, &KLineEdit::textEdited, this, &ReferenceWidget::entryIdManuallyChanged);
    connect(entryType->lineEdit(), &KLineEdit::textChanged, this, &ReferenceWidget::gotModified);

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
    return QString();
}

QIcon ReferenceWidget::icon()
{
    return QIcon();
}

bool ReferenceWidget::canEdit(const Element *element)
{
    return Entry::isEntry(*element) || Macro::isMacro(*element);
}

void ReferenceWidget::setOriginalElement(const QSharedPointer<Element> &orig)
{
    m_element = orig;
}

QString ReferenceWidget::currentId() const
{
    return entryId->text();
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
    entryId->setClearButtonEnabled(true);
    label = new QLabel(i18n("Id:"), this);
    label->setBuddy(entryId);
    layout->addWidget(label);
    layout->addWidget(entryId);

    const BibTeXEntries *be = BibTeXEntries::self();
    for (const auto &ed : const_cast<const BibTeXEntries &>(*be))
        entryType->addItem(ed.label, ed.upperCamelCase);
    /// Sort the combo box locale-aware. Thus we need a SortFilterProxyModel
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(entryType);
    proxy->setSortLocaleAware(true);
    proxy->setSourceModel(entryType->model());
    entryType->model()->setParent(proxy);
    entryType->setModel(proxy);
    entryType->model()->sort(0);

    /// Button with a menu listing a set of preconfigured id suggestions
    buttonSuggestId = new QPushButton(QIcon::fromTheme(QStringLiteral("view-filter")), QStringLiteral(""), this);
    buttonSuggestId->setToolTip(i18n("Select a suggested id for this entry"));
    layout->addWidget(buttonSuggestId);
    QMenu *suggestionsMenu = new QMenu(buttonSuggestId);
    buttonSuggestId->setMenu(suggestionsMenu);

    connect(entryType->lineEdit(), &KLineEdit::textChanged, this, &ReferenceWidget::gotModified);
    connect(entryId, &KLineEdit::textEdited, this, &ReferenceWidget::entryIdManuallyChanged);
    connect(entryType->lineEdit(), &KLineEdit::textChanged, this, &ReferenceWidget::entryTypeChanged);
    connect(suggestionsMenu, &QMenu::aboutToShow, this, &ReferenceWidget::prepareSuggestionsMenu);
}

void ReferenceWidget::prepareSuggestionsMenu()
{
    /// Collect information on the current entry as it is edited
    QSharedPointer<Entry> guiDataEntry(new Entry());
    m_applyElement->apply(guiDataEntry);
    QSharedPointer<Entry> crossrefResolvedEntry(Entry::resolveCrossref(*guiDataEntry.data(), m_file));

    static const IdSuggestions *idSuggestions = new IdSuggestions();
    QMenu *suggestionsMenu = buttonSuggestId->menu();
    suggestionsMenu->clear();

    /// Keep track of shown suggestions to avoid duplicates
    QSet<QString> knownIdSuggestion;
    const QString defaultSuggestion = idSuggestions->defaultFormatId(*crossrefResolvedEntry.data());

    const auto formatIdList = idSuggestions->formatIdList(*crossrefResolvedEntry.data());
    for (const QString &suggestionBase : formatIdList) {
        bool isDefault = suggestionBase == defaultSuggestion;
        QString suggestion = suggestionBase;

        /// Test for duplicate ids, use fallback ids with numeric suffix
        if (m_file != nullptr && m_file->containsKey(suggestion)) {
            int suffix = 2;
            while (m_file->containsKey(suggestion = suggestionBase + QChar('_') + QString::number(suffix)))
                ++suffix;
        }

        /// Keep track of shown suggestions to avoid duplicates
        if (knownIdSuggestion.contains(suggestion)) continue;
        else knownIdSuggestion.insert(suggestion);

        /// Create action for suggestion, use icon depending if default or not
        QAction *suggestionAction = new QAction(suggestion, suggestionsMenu);
        suggestionAction->setIcon(QIcon::fromTheme(isDefault ? QStringLiteral("favorites") : QStringLiteral("view-filter")));

        /// Mesh action into GUI
        suggestionsMenu->addAction(suggestionAction);
        connect(suggestionAction, &QAction::triggered, this, &ReferenceWidget::insertSuggestionFromAction);
        /// Remember suggestion string for time when action gets triggered
        suggestionAction->setProperty(PropertyIdSuggestion, suggestion);
    }
}

void ReferenceWidget::insertSuggestionFromAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action != nullptr) {
        const QString suggestion = action->property(PropertyIdSuggestion).toString();
        entryId->setText(suggestion);
    }
}

void ReferenceWidget::entryIdManuallyChanged()
{
    m_entryIdManuallySet = true;
    gotModified();
}

void ReferenceWidget::setEntryIdByDefault()
{
    if (isReadOnly) {
        /// Never set the suggestion automatically if in read-only mode
        return;
    }

    if (m_entryIdManuallySet) {
        /// If user changed entry id manually,
        /// do not overwrite it by a default value
        return;
    }

    static const IdSuggestions *idSuggestions = new IdSuggestions();
    /// If there is a default suggestion format set ...
    if (idSuggestions->hasDefaultFormat())  {
        /// Collect information on the current entry as it is edited
        QSharedPointer<Entry> guiDataEntry(new Entry());
        m_applyElement->apply(guiDataEntry);
        QSharedPointer<Entry> crossrefResolvedEntry(Entry::resolveCrossref(*guiDataEntry.data(), m_file));
        /// Determine default suggestion based on current data
        const QString defaultSuggestion = idSuggestions->defaultFormatId(*crossrefResolvedEntry.data());

        if (!defaultSuggestion.isEmpty()) {
            disconnect(entryId, &KLineEdit::textEdited, this, &ReferenceWidget::entryIdManuallyChanged);
            /// Apply default suggestion to widget
            entryId->setText(defaultSuggestion);
            connect(entryId, &KLineEdit::textEdited, this, &ReferenceWidget::entryIdManuallyChanged);
        }
    }
}

QString ReferenceWidget::computeType() const
{
    const BibTeXEntries *be = BibTeXEntries::self();
    if (entryType->currentIndex() < 0 || entryType->lineEdit()->isModified())
        return be->format(entryType->lineEdit()->text(), KBibTeX::cUpperCamelCase);
    else
        return entryType->itemData(entryType->currentIndex()).toString();
}

FilesWidget::FilesWidget(QWidget *parent)
        : ElementWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    fileList = new FieldInput(KBibTeX::UrlList, KBibTeX::tfVerbatim /* eventually ignored, see constructor of UrlListEdit */, KBibTeX::tfVerbatim /* eventually ignored, see constructor of UrlListEdit */, this);
    fileList->setFieldKey(QStringLiteral("^external"));
    layout->addWidget(fileList);
    connect(fileList, &FieldInput::modified, this, &FilesWidget::gotModified);
}

bool FilesWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (entry.isNull()) return false;

    for (const QString &keyStem : keyStart)
        for (int i = 1; i < 32; ++i) {  /// FIXME replace number by constant
            const QString key = i > 1 ? keyStem + QString::number(i) : keyStem;
            entry->remove(key);
        }

    Value combinedValue;
    fileList->apply(combinedValue);

    Value urlValue, doiValue, localFileValue;
    urlValue.reserve(combinedValue.size());
    doiValue.reserve(combinedValue.size());
    localFileValue.reserve(combinedValue.size());

    for (const auto &valueItem : const_cast<const Value &>(combinedValue)) {
        const QSharedPointer<const VerbatimText> verbatimText = valueItem.dynamicCast<const VerbatimText>();
        if (!verbatimText.isNull()) {
            const QString text = verbatimText->text();
            QRegularExpressionMatch match;
            if ((match = KBibTeX::urlRegExp.match(text)).hasMatch()) {
                /// add full URL
                VerbatimText *newVT = new VerbatimText(match.captured(0));
                /// test for duplicates
                if (urlValue.contains(*newVT))
                    delete newVT;
                else
                    urlValue.append(QSharedPointer<VerbatimText>(newVT));
            } else if ((match = KBibTeX::doiRegExp.match(text)).hasMatch()) {
                /// add DOI
                VerbatimText *newVT = new VerbatimText(match.captured(0));
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
    for (const QString &keyStem : keyStart)
        for (int i = 1; i < 32; ++i) {  /// FIXME replace number by constant
            const QString key = i > 1 ? keyStem + QString::number(i) : keyStem;
            const Value &value = entry->operator [](key);
            for (const auto &valueItem : const_cast<const Value &>(value))
                combinedValue.append(valueItem);
        }
    fileList->setElement(element.data());
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

QIcon FilesWidget::icon()
{
    return QIcon::fromTheme(QStringLiteral("emblem-symbolic-link"));
}

void FilesWidget::setFile(const File *file)
{
    ElementWidget::setFile(file);
    fileList->setFile(file);
}

bool FilesWidget::canEdit(const Element *element)
{
    return Entry::isEntry(*element);
}

const QStringList FilesWidget::keyStart {Entry::ftUrl, QStringLiteral("postscript"), Entry::ftLocalFile, Entry::ftDOI, Entry::ftFile, QStringLiteral("ee"), QStringLiteral("biburl")};



OtherFieldsWidget::OtherFieldsWidget(const QStringList &blacklistedFields, QWidget *parent)
        : ElementWidget(parent), blackListed(blacklistedFields)
{
    internalEntry = QSharedPointer<Entry>(new Entry());
    createGUI();
}

OtherFieldsWidget::~OtherFieldsWidget()
{
    delete fieldContent;
}

bool OtherFieldsWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; /// never save data if in read-only mode

    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    if (entry.isNull()) return false;

    for (const QString &key : const_cast<const QStringList &>(deletedKeys))
        entry->remove(key);
    for (const QString &key : const_cast<const QStringList &>(modifiedKeys)) {
        entry->remove(key);
        entry->insert(key, internalEntry->value(key));
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

QIcon OtherFieldsWidget::icon()
{
    return QIcon::fromTheme(QStringLiteral("other"));
}

bool OtherFieldsWidget::canEdit(const Element *element)
{
    return Entry::isEntry(*element);
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
    bool somethingSelected = item != nullptr;
    buttonDelete->setEnabled(somethingSelected && !isReadOnly);
    if (somethingSelected) {
        currentUrl = QUrl(item->text(1));
        validUrl = currentUrl.isValid() && currentUrl.isLocalFile() & QFileInfo::exists(currentUrl.toLocalFile());
        if (!validUrl) {
            const QRegularExpressionMatch urlRegExpMatch = KBibTeX::urlRegExp.match(item->text(1));
            if (urlRegExpMatch.hasMatch()) {
                currentUrl = QUrl(urlRegExpMatch.captured(0));
                validUrl = currentUrl.isValid();
                buttonOpen->setEnabled(validUrl);
            }
        }
    }

    if (!validUrl)
        currentUrl = QUrl();
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

    Q_ASSERT_X(otherFieldsList->currentItem() != nullptr, "OtherFieldsWidget::actionDelete", "otherFieldsList->currentItem() is NULL");
    QString key = otherFieldsList->currentItem()->text(0);
    if (!deletedKeys.contains(key)) deletedKeys << key;

    internalEntry->remove(key);
    updateList();
    updateGUI();
    listCurrentChanged(otherFieldsList->currentItem(), nullptr);

    gotModified();
}

void OtherFieldsWidget::actionOpen()
{
    if (currentUrl.isValid()) {
        /// Guess mime type for url to open
        QMimeType mimeType = FileInfo::mimeTypeForUrl(currentUrl);
        const QString mimeTypeName = mimeType.name();
        /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < 0x051f00 // < 5.31.0
        KRun::runUrl(currentUrl, mimeTypeName, this, false, false);
#else // KIO_VERSION < 0x051f00 // >= 5.31.0
        KRun::runUrl(currentUrl, mimeTypeName, this, KRun::RunFlags());
#endif // KIO_VERSION < 0x051f00
    }
}

void OtherFieldsWidget::createGUI()
{
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
    layout->addWidget(label, 0, 0, 1, 1);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));

    fieldName = new KLineEdit(this);
    layout->addWidget(fieldName, 0, 1, 1, 1);
    label->setBuddy(fieldName);

    buttonAddApply = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add"), this);
    buttonAddApply->setEnabled(false);
    layout->addWidget(buttonAddApply, 0, 2, 1, 1);

    label = new QLabel(i18n("Content:"), this);
    layout->addWidget(label, 1, 0, 1, 1);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    fieldContent = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, KBibTeX::tfSource, this);
    layout->addWidget(fieldContent, 1, 1, 1, 2);
    label->setBuddy(fieldContent->buddy());

    label = new QLabel(i18n("List:"), this);
    layout->addWidget(label, 2,  0, 1, 1);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));

    otherFieldsList = new QTreeWidget(this);
    otherFieldsList->setHeaderLabels(QStringList {i18n("Key"), i18n("Value")});
    otherFieldsList->setRootIsDecorated(false);
    layout->addWidget(otherFieldsList, 2, 1, 3, 1);
    label->setBuddy(otherFieldsList);

    buttonDelete = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Delete"), this);
    buttonDelete->setEnabled(false);
    layout->addWidget(buttonDelete,  2, 2, 1, 1);
    buttonOpen = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open"), this);
    buttonOpen->setEnabled(false);
    layout->addWidget(buttonOpen, 3, 2, 1, 1);

    connect(otherFieldsList, &QTreeWidget::itemActivated, this, &OtherFieldsWidget::listElementExecuted);
    connect(otherFieldsList, &QTreeWidget::currentItemChanged, this, &OtherFieldsWidget::listCurrentChanged);
    connect(otherFieldsList, &QTreeWidget::itemSelectionChanged, this, &OtherFieldsWidget::updateGUI);
    connect(fieldName, &KLineEdit::textEdited, this, &OtherFieldsWidget::updateGUI);
    connect(buttonAddApply, &QPushButton::clicked, this, &OtherFieldsWidget::actionAddApply);
    connect(buttonDelete, &QPushButton::clicked, this, &OtherFieldsWidget::actionDelete);
    connect(buttonOpen, &QPushButton::clicked, this, &OtherFieldsWidget::actionOpen);
}

void OtherFieldsWidget::updateList()
{
    const QString selText = otherFieldsList->selectedItems().isEmpty() ? QString() : otherFieldsList->selectedItems().first()->text(0);
    const QString curText = otherFieldsList->currentItem() == nullptr ? QString() : otherFieldsList->currentItem()->text(0);
    otherFieldsList->clear();

    for (Entry::ConstIterator it = internalEntry->constBegin(); it != internalEntry->constEnd(); ++it)
        if (!blackListed.contains(it.key().toLower())) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, it.key());
            item->setText(1, PlainTextValue::text(it.value()));
            item->setIcon(0, QIcon::fromTheme(QStringLiteral("entry"))); // FIXME
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
        buttonAddApply->setIcon(internalEntry->contains(key) ? QIcon::fromTheme(QStringLiteral("document-edit")) : QIcon::fromTheme(QStringLiteral("list-add")));
    }
}

MacroWidget::MacroWidget(QWidget *parent)
        : ElementWidget(parent)
{
    createGUI();
}

MacroWidget::~MacroWidget()
{
    delete fieldInputValue;
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

QIcon MacroWidget::icon()
{
    return QIcon::fromTheme(QStringLiteral("macro"));
}

bool MacroWidget::canEdit(const Element *element)
{
    return Macro::isMacro(*element);
}

void MacroWidget::createGUI()
{
    QBoxLayout *layout = new QHBoxLayout(this);

    QLabel *label = new QLabel(i18n("Value:"), this);
    layout->addWidget(label, 0);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    fieldInputValue = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfSource, this);
    layout->addWidget(fieldInputValue, 1);
    label->setBuddy(fieldInputValue->buddy());

    connect(fieldInputValue, &FieldInput::modified, this, &MacroWidget::gotModified);
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

QIcon PreambleWidget::icon()
{
    return QIcon::fromTheme(QStringLiteral("preamble"));
}

bool PreambleWidget::canEdit(const Element *element)
{
    return Preamble::isPreamble(*element);
}

void PreambleWidget::createGUI()
{
    QBoxLayout *layout = new QHBoxLayout(this);

    QLabel *label = new QLabel(i18n("Value:"), this);
    layout->addWidget(label, 0);
    label->setAlignment((Qt::Alignment)label->style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    fieldInputValue = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, KBibTeX::tfSource, this); // FIXME: other editing modes beyond Source applicable?
    layout->addWidget(fieldInputValue, 1);
    label->setBuddy(fieldInputValue->buddy());

    connect(fieldInputValue, &FieldInput::modified, this, &PreambleWidget::gotModified);
}


class SourceWidget::SourceWidgetTextEdit : public KTextEdit
{
    Q_OBJECT

public:
    SourceWidgetTextEdit(QWidget *parent)
            : KTextEdit(parent) {
        /// nothing
    }

protected:
    void dropEvent(QDropEvent *event) override {
        FileImporterBibTeX importer(this);
        QScopedPointer<File> file(importer.fromString(event->mimeData()->text()));
        if (!file.isNull() && file->count() == 1) {
            FileExporterBibTeX exporter(this);
            document()->setPlainText(exporter.toString(file->first(), file.data()));
        } else
            KTextEdit::dropEvent(event);
    }
};

class SourceWidget::Private
{
public:
    QPushButton *buttonRestore;
    FileImporterBibTeX *importerBibTeX;

    Private(SourceWidget *parent)
            : buttonRestore(nullptr), importerBibTeX(new FileImporterBibTeX(parent)) {
        /// nothing
    }
};

SourceWidget::SourceWidget(QWidget *parent)
        : ElementWidget(parent), d(new SourceWidget::Private(this))
{
    createGUI();
}

SourceWidget::~SourceWidget()
{
    delete sourceEdit;
    delete d;
}

bool SourceWidget::apply(QSharedPointer<Element> element) const
{
    if (isReadOnly) return false; ///< never save data if in read-only mode

    const QString text = sourceEdit->document()->toPlainText();
    const QScopedPointer<const File> file(d->importerBibTeX->fromString(text));
    if (file.isNull() || file->count() != 1) return false;


    QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
    QSharedPointer<Entry> readEntry = file->first().dynamicCast<Entry>();
    if (!readEntry.isNull() && !entry.isNull()) {
        entry->operator =(*readEntry.data()); //entry = readEntry;
        return true;
    } else {
        QSharedPointer<Macro> macro = element.dynamicCast<Macro>();
        QSharedPointer<Macro> readMacro = file->first().dynamicCast<Macro>();
        if (!readMacro.isNull() && !macro.isNull()) {
            macro->operator =(*readMacro.data());
            return true;
        } else {
            QSharedPointer<Preamble> preamble = element.dynamicCast<Preamble>();
            QSharedPointer<Preamble> readPreamble = file->first().dynamicCast<Preamble>();
            if (!readPreamble.isNull() && !preamble.isNull()) {
                preamble->operator =(*readPreamble.data());
                return true;
            } else {
                qCWarning(LOG_KBIBTEX_GUI) << "Do not know how to apply source code";
                return false;
            }
        }
    }
}

bool SourceWidget::reset(QSharedPointer<const Element> element)
{
    /// if signals are not deactivated, the "modified" signal would be emitted when
    /// resetting the widget's value
    disconnect(sourceEdit, &SourceWidget::SourceWidgetTextEdit::textChanged, this, &SourceWidget::gotModified);

    FileExporterBibTeX exporter(this);
    exporter.setEncoding(QStringLiteral("utf-8"));
    const QString exportedText = exporter.toString(element, m_file);
    if (!exportedText.isEmpty()) {
        originalText = exportedText;
        sourceEdit->document()->setPlainText(originalText);
    }

    connect(sourceEdit, &SourceWidget::SourceWidgetTextEdit::textChanged, this, &SourceWidget::gotModified);

    return !exportedText.isEmpty();
}

void SourceWidget::setReadOnly(bool isReadOnly)
{
    ElementWidget::setReadOnly(isReadOnly);

    d->buttonRestore->setEnabled(!isReadOnly);
    sourceEdit->setReadOnly(isReadOnly);
}

QString SourceWidget::label()
{
    return i18n("Source");
}

QIcon SourceWidget::icon()
{
    return QIcon::fromTheme(QStringLiteral("code-context"));
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
    sourceEdit->document()->setDefaultFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    sourceEdit->setTabStopWidth(QFontMetrics(sourceEdit->font()).averageCharWidth() * 4);

    d->buttonRestore = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-undo")), i18n("Restore"), this);
    layout->addWidget(d->buttonRestore, 1, 1, 1, 1);
    connect(d->buttonRestore, &QPushButton::clicked, this, static_cast<void(SourceWidget::*)()>(&SourceWidget::reset));
    connect(sourceEdit, &SourceWidget::SourceWidgetTextEdit::textChanged, this, &SourceWidget::gotModified);
}

void SourceWidget::reset()
{
    /// if signals are not deactivated, the "modified" signal would be emitted when
    /// resetting the widget's value
    disconnect(sourceEdit, &SourceWidget::SourceWidgetTextEdit::textChanged, this, &SourceWidget::gotModified);

    sourceEdit->document()->setPlainText(originalText);
    setModified(false);

    connect(sourceEdit, &SourceWidget::SourceWidgetTextEdit::textChanged, this, &SourceWidget::gotModified);
}

#include "elementwidgets.moc"
