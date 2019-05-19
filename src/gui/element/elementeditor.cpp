/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "elementeditor.h"

#include <typeinfo>

#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QBuffer>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>
#include <QMenu>
#include <QScrollArea>
#include <QPushButton>

#include <KMessageBox>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <NotificationHub>
#include <Entry>
#include <Comment>
#include <Macro>
#include <Preamble>
#include <Element>
#include <File>
#include <CheckBibTeX>
#include "elementwidgets.h"
#include "widgets/hidingtabwidget.h"
#include "widgets/menulineedit.h"

class ElementEditor::ElementEditorPrivate : public ElementEditor::ApplyElementInterface
{
private:
    typedef QVector<ElementWidget *> WidgetList;
    WidgetList widgets;
    const File *file;
    QSharedPointer<Entry> internalEntry;
    QSharedPointer<Macro> internalMacro;
    QSharedPointer<Preamble> internalPreamble;
    QSharedPointer<Comment> internalComment;
    ElementEditor *p;
    ElementWidget *previousWidget;
    ReferenceWidget *referenceWidget;
    SourceWidget *sourceWidget;
    QPushButton *buttonCheckWithBibTeX;

    /// Settings management through a push button with menu
    KSharedConfigPtr config;
    QPushButton *buttonOptions;
    QAction *actionForceShowAllWidgets, *actionLimitKeyboardTabStops;

public:
    QSharedPointer<Element> element;
    HidingTabWidget *tab;
    bool elementChanged, elementUnapplied;

    ElementEditorPrivate(bool scrollable, ElementEditor *parent)
            : file(nullptr), p(parent), previousWidget(nullptr), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), elementChanged(false), elementUnapplied(false) {
        internalEntry = QSharedPointer<Entry>();
        internalMacro = QSharedPointer<Macro>();
        internalComment = QSharedPointer<Comment>();
        internalPreamble = QSharedPointer<Preamble>();

        createGUI(scrollable);
    }

    ~ElementEditorPrivate() override {
        clearWidgets();
    }

    void clearWidgets() {
        for (int i = widgets.count() - 1; i >= 0; --i) {
            QWidget *w = widgets[i];
            w->deleteLater();
        }
        widgets.clear();
    }

    void setElement(QSharedPointer<Element> element, const File *file) {
        this->element = element;
        this->file = file;
        referenceWidget->setOriginalElement(element);
        updateTabVisibility();
    }

    void addTabWidgets() {
        for (const auto &etl : EntryLayout::instance()) {
            ElementWidget *widget = new EntryConfiguredWidget(etl, tab);
            connect(widget, &ElementWidget::modified, p, &ElementEditor::childModified);
            widgets << widget;
            if (previousWidget == nullptr)
                previousWidget = widget; ///< memorize the first tab
            int index = tab->addTab(widget, widget->icon(), widget->label());
            tab->hideTab(index);
        }

        ElementWidget *widget = new PreambleWidget(tab);
        connect(widget, &ElementWidget::modified, p, &ElementEditor::childModified);
        widgets << widget;
        int index = tab->addTab(widget, widget->icon(), widget->label());
        tab->hideTab(index);

        widget = new MacroWidget(tab);
        connect(widget, &ElementWidget::modified, p, &ElementEditor::childModified);
        widgets << widget;
        index = tab->addTab(widget, widget->icon(), widget->label());
        tab->hideTab(index);

        FilesWidget *filesWidget = new FilesWidget(tab);
        connect(filesWidget, &FilesWidget::modified, p, &ElementEditor::childModified);
        widgets << filesWidget;
        index = tab->addTab(filesWidget, filesWidget->icon(), filesWidget->label());
        tab->hideTab(index);

        QStringList blacklistedFields;

        /// blacklist fields covered by EntryConfiguredWidget
        for (const auto &etl : EntryLayout::instance())
            for (const auto &sfl : const_cast<const QList<SingleFieldLayout> &>(etl->singleFieldLayouts))
                blacklistedFields << sfl.bibtexLabel;

        /// blacklist fields covered by FilesWidget
        blacklistedFields << QString(Entry::ftUrl) << QString(Entry::ftLocalFile) << QString(Entry::ftFile) << QString(Entry::ftDOI) << QStringLiteral("ee") << QStringLiteral("biburl") << QStringLiteral("postscript");
        for (int i = 2; i < 256; ++i) // FIXME replace number by constant
            blacklistedFields << QString(Entry::ftUrl) + QString::number(i) << QString(Entry::ftLocalFile) + QString::number(i) << QString(Entry::ftFile) + QString::number(i) <<  QString(Entry::ftDOI) + QString::number(i) << QStringLiteral("ee") + QString::number(i) << QStringLiteral("postscript") + QString::number(i);

        widget = new OtherFieldsWidget(blacklistedFields, tab);
        connect(widget, &ElementWidget::modified, p, &ElementEditor::childModified);
        widgets << widget;
        index = tab->addTab(widget, widget->icon(), widget->label());
        tab->hideTab(index);

        sourceWidget = new SourceWidget(tab);
        connect(sourceWidget, &ElementWidget::modified, p, &ElementEditor::childModified);
        widgets << sourceWidget;
        index = tab->addTab(sourceWidget, sourceWidget->icon(), sourceWidget->label());
        tab->hideTab(index);
    }

    void createGUI(bool scrollable) {
        /// load configuration for options push button
        static const QString configGroupName = QStringLiteral("User Interface");
        static const QString keyEnableAllWidgets = QStringLiteral("EnableAllWidgets");
        KConfigGroup configGroup(config, configGroupName);
        const bool showAll = configGroup.readEntry(keyEnableAllWidgets, true);
        const bool limitKeyboardTabStops = configGroup.readEntry(MenuLineEdit::keyLimitKeyboardTabStops, false);

        QBoxLayout *vLayout = new QVBoxLayout(p);

        referenceWidget = new ReferenceWidget(p);
        referenceWidget->setApplyElementInterface(this);
        connect(referenceWidget, &ElementWidget::modified, p, &ElementEditor::childModified);
        connect(referenceWidget, &ReferenceWidget::entryTypeChanged, p, &ElementEditor::updateReqOptWidgets);
        vLayout->addWidget(referenceWidget, 0);
        widgets << referenceWidget;

        if (scrollable) {
            QScrollArea *sa = new QScrollArea(p);
            tab = new HidingTabWidget(sa);
            sa->setFrameStyle(0);
            sa->setWidget(tab);
            sa->setWidgetResizable(true);
            sa->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            vLayout->addWidget(sa, 10);
        } else {
            tab = new HidingTabWidget(p);
            vLayout->addWidget(tab, 10);
        }

        QBoxLayout *hLayout = new QHBoxLayout();
        vLayout->addLayout(hLayout, 0);

        /// Push button with menu to toggle various options
        buttonOptions = new QPushButton(QIcon::fromTheme(QStringLiteral("configure")), i18n("Options"), p);
        hLayout->addWidget(buttonOptions, 0);
        QMenu *menuOptions = new QMenu(buttonOptions);
        buttonOptions->setMenu(menuOptions);

        /// Option to show all fields or only those require for current entry type
        actionForceShowAllWidgets = menuOptions->addAction(i18n("Show all fields"), p, SLOT(updateReqOptWidgets()));
        actionForceShowAllWidgets->setCheckable(true);
        actionForceShowAllWidgets->setChecked(showAll);

        /// Option to disable tab key focus to reach/visit various non-editable widgets
        actionLimitKeyboardTabStops = menuOptions->addAction(i18n("Tab key visits only editable fields"), p, SLOT(limitKeyboardTabStops()));
        actionLimitKeyboardTabStops->setCheckable(true);
        actionLimitKeyboardTabStops->setChecked(limitKeyboardTabStops);

        hLayout->addStretch(10);

        buttonCheckWithBibTeX = new QPushButton(QIcon::fromTheme(QStringLiteral("tools-check-spelling")), i18n("Check with BibTeX"), p);
        hLayout->addWidget(buttonCheckWithBibTeX, 0);
        connect(buttonCheckWithBibTeX, &QPushButton::clicked, p, &ElementEditor::checkBibTeX);

        addTabWidgets();
    }

    void updateTabVisibility() {
        disconnect(tab, &HidingTabWidget::currentChanged, p, &ElementEditor::tabChanged);
        if (element.isNull()) {
            p->setEnabled(false);
        } else {
            p->setEnabled(true);
            int firstEnabledTab = 1024;

            for (ElementWidget *widget : const_cast<const WidgetList &>(widgets)) {
                const int index = tab->indexOf(widget);
                const bool canEdit = widget->canEdit(element.data());

                if (widget == referenceWidget) {
                    /// Reference widget
                    widget->setVisible(canEdit);
                    widget->setEnabled(canEdit);
                } else {
                    if (canEdit) tab->showTab(widget);
                    else if (index >= 0) tab->hideTab(index);
                    if (canEdit && index >= 0 && index < firstEnabledTab)
                        firstEnabledTab = index;
                }
            }
            if (firstEnabledTab < 1024)
                tab->setCurrentIndex(firstEnabledTab);
        }
        connect(tab, &HidingTabWidget::currentChanged, p, &ElementEditor::tabChanged);
    }

    /**
     * If this element editor makes use of a reference widget
     * (e.g. where entry type and entry id/macro key can be edited),
     * then return the current value of the entry id/macro key
     * editing widget.
     * Otherwise, return an empty string.
     *
     * @return Current value of entry id/macro key if any, otherwise empty string
     */
    QString currentId() const {
        if (referenceWidget != nullptr)
            return referenceWidget->currentId();
        return QString();
    }

    void setCurrentId(const QString &newId) {
        if (referenceWidget != nullptr)
            return referenceWidget->setCurrentId(newId);
    }

    /**
     * Return the current File object set for this element editor.
     * May be NULL if nothing has been set or if it has been cleared.
     *
     * @return Current File object, may be nullptr
     */
    const File *currentFile() const {
        return file;
    }

    void apply() {
        elementChanged = true;
        elementUnapplied = false;
        apply(element);
    }

    void apply(QSharedPointer<Element> element) override {
        QSharedPointer<Entry> e = element.dynamicCast<Entry>();
        QSharedPointer<Macro> m = e.isNull() ? element.dynamicCast<Macro>() : QSharedPointer<Macro>();
        QSharedPointer<Comment> c = e.isNull() && m.isNull() ? element.dynamicCast<Comment>() : QSharedPointer<Comment>();
        QSharedPointer<Preamble> p = e.isNull() && m.isNull() && c.isNull() ? element.dynamicCast<Preamble>() : QSharedPointer<Preamble>();

        if (tab->currentWidget() == sourceWidget) {
            /// Very simple if source view is active: BibTeX code contains
            /// all necessary data
            if (!e.isNull())
                sourceWidget->setElementClass(SourceWidget::elementEntry);
            else if (!m.isNull())
                sourceWidget->setElementClass(SourceWidget::elementMacro);
            else if (!p.isNull())
                sourceWidget->setElementClass(SourceWidget::elementPreamble);
            else
                sourceWidget->setElementClass(SourceWidget::elementInvalid);
            sourceWidget->apply(element);
        } else {
            /// Start by assigning the current internal element's
            /// data to the output element
            if (!e.isNull())
                *e = *internalEntry;
            else {
                if (!m.isNull())
                    *m = *internalMacro;
                else {
                    if (!c.isNull())
                        *c = *internalComment;
                    else {
                        if (!p.isNull())
                            *p = *internalPreamble;
                        else
                            Q_ASSERT_X(element.isNull(), "ElementEditor::ElementEditorPrivate::apply(QSharedPointer<Element> element)", "element is not NULL but could not be cast on a valid Element sub-class");
                    }
                }
            }

            /// The internal element may be outdated (only updated on tab switch),
            /// so apply the reference widget's data on the output element
            if (referenceWidget != nullptr)
                referenceWidget->apply(element);
            /// The internal element may be outdated (only updated on tab switch),
            /// so apply the current widget's data on the output element
            ElementWidget *currentElementWidget = qobject_cast<ElementWidget *>(tab->currentWidget());
            if (currentElementWidget != nullptr)
                currentElementWidget->apply(element);
        }
    }

    bool validate(QWidget **widgetWithIssue, QString &message) const override {
        if (tab->currentWidget() == sourceWidget) {
            /// Source widget must check its textual content for being valid BibTeX code
            return sourceWidget->validate(widgetWithIssue, message);
        } else {
            /// All widgets except for the source widget must validate their values
            for (WidgetList::ConstIterator it = widgets.begin(); it != widgets.end(); ++it) {
                if ((*it) == sourceWidget) continue;
                const bool v = (*it)->validate(widgetWithIssue, message);
                /// A single widget failing to validate lets the whole validation fail
                if (!v) return false;
            }
            return true;
        }
    }

    void reset() {
        elementChanged = false;
        elementUnapplied = false;
        reset(element);

        /// show checkbox to enable all fields only if editing an entry
        actionForceShowAllWidgets->setVisible(!internalEntry.isNull());
        /// Disable widgets if necessary
        if (!actionForceShowAllWidgets->isChecked())
            updateReqOptWidgets();
    }

    void reset(QSharedPointer<const Element> element) {
        for (WidgetList::Iterator it = widgets.begin(); it != widgets.end(); ++it) {
            (*it)->setFile(file);
            (*it)->reset(element);
            (*it)->setModified(false);
        }

        QSharedPointer<const Entry> e = element.dynamicCast<const Entry>();
        if (!e.isNull()) {
            internalEntry = QSharedPointer<Entry>(new Entry(*e.data()));
            sourceWidget->setElementClass(SourceWidget::elementEntry);
        } else {
            QSharedPointer<const Macro> m = element.dynamicCast<const Macro>();
            if (!m.isNull()) {
                internalMacro = QSharedPointer<Macro>(new Macro(*m.data()));
                sourceWidget->setElementClass(SourceWidget::elementMacro);
            } else {
                QSharedPointer<const Comment> c = element.dynamicCast<const Comment>();
                if (!c.isNull()) {
                    internalComment = QSharedPointer<Comment>(new Comment(*c.data()));
                    sourceWidget->setElementClass(SourceWidget::elementComment);
                } else {
                    QSharedPointer<const Preamble> p = element.dynamicCast<const Preamble>();
                    if (!p.isNull()) {
                        internalPreamble = QSharedPointer<Preamble>(new Preamble(*p.data()));
                        sourceWidget->setElementClass(SourceWidget::elementPreamble);
                    } else
                        Q_ASSERT_X(element.isNull(), "ElementEditor::ElementEditorPrivate::reset(QSharedPointer<const Element> element)", "element is not NULL but could not be cast on a valid Element sub-class");
                }
            }
        }

        buttonCheckWithBibTeX->setEnabled(!internalEntry.isNull());
    }

    void setReadOnly(bool isReadOnly) {
        for (WidgetList::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->setReadOnly(isReadOnly);
    }

    void updateReqOptWidgets() {
        /// this function is only relevant if editing an entry (and not e.g. a comment)
        if (internalEntry.isNull()) return; /// quick-and-dirty test if editing an entry

        /// make a temporary snapshot of the current state
        QSharedPointer<Entry> tempEntry = QSharedPointer<Entry>(new Entry());
        apply(tempEntry);

        /// update the enabled/disabled state of required and optional widgets/fields
        bool forceVisible = actionForceShowAllWidgets->isChecked();
        for (ElementWidget *elementWidget : const_cast<const WidgetList &>(widgets)) {
            elementWidget->showReqOptWidgets(forceVisible, tempEntry->type());
        }

        /// save configuration
        static const QString configGroupName = QStringLiteral("User Interface");
        static const QString keyEnableAllWidgets = QStringLiteral("EnableAllWidgets");
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(keyEnableAllWidgets, actionForceShowAllWidgets->isChecked());
        config->sync();
    }

    void limitKeyboardTabStops() {
        /// save configuration
        static const QString configGroupName = QStringLiteral("User Interface");
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(MenuLineEdit::keyLimitKeyboardTabStops, actionLimitKeyboardTabStops->isChecked());
        config->sync();

        /// notify all listening MenuLineEdit widgets to change their behavior
        NotificationHub::publishEvent(MenuLineEdit::MenuLineConfigurationChangedEvent);
    }

    void switchTo(QWidget *futureTab) {
        /// Switched from source widget to another widget?
        const bool isToSourceWidget = futureTab == sourceWidget;
        /// Switch from some widget to the source widget?
        const bool isFromSourceWidget = previousWidget == sourceWidget;
        /// Interprete future widget as an ElementWidget
        ElementWidget *futureWidget = qobject_cast<ElementWidget *>(futureTab);
        /// Past and future ElementWidget values are valid?
        if (previousWidget != nullptr && futureWidget != nullptr) {
            /// Assign to temp wihch internal variable holds current state
            QSharedPointer<Element> temp;
            if (!internalEntry.isNull())
                temp = internalEntry;
            else if (!internalMacro.isNull())
                temp = internalMacro;
            else if (!internalComment.isNull())
                temp = internalComment;
            else if (!internalPreamble.isNull())
                temp = internalPreamble;
            Q_ASSERT_X(!temp.isNull(), "void ElementEditor::ElementEditorPrivate::switchTo(QWidget *newTab)", "temp is NULL");

            /// Past widget writes its state to the internal state
            previousWidget->apply(temp);
            /// Before switching to source widget, store internally reference widget's state
            if (isToSourceWidget && referenceWidget != nullptr) referenceWidget->apply(temp);
            /// Tell future widget to initialize itself based on internal state
            futureWidget->reset(temp);
            /// When switchin from source widget to another widget, initialize reference widget
            if (isFromSourceWidget && referenceWidget != nullptr)
                referenceWidget->reset(temp);
        }
        previousWidget = futureWidget;

        /// Enable/disable tabs
        for (WidgetList::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->setEnabled(!isToSourceWidget || *it == futureTab);
    }

    /**
      * Test current entry if it compiles with BibTeX.
      * Show warnings and errors in message box.
      */
    void checkBibTeX() {
        /// disable GUI under process
        p->setEnabled(false);
        QSharedPointer<Entry> entry = QSharedPointer<Entry>(new Entry());
        apply(entry);
        CheckBibTeX::checkBibTeX(entry, file, p);
        p->setEnabled(true);
    }

    void setModified(bool newIsModified) {
        for (WidgetList::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->setModified(newIsModified);
    }

    void referenceWidgetSetEntryIdByDefault() {
        referenceWidget->setEntryIdByDefault();
    }
};

ElementEditor::ElementEditor(bool scrollable, QWidget *parent)
        : QWidget(parent), d(new ElementEditorPrivate(scrollable, this))
{
    connect(d->tab, &HidingTabWidget::currentChanged, this, &ElementEditor::tabChanged);
}

ElementEditor::~ElementEditor()
{
    disconnect(d->tab, &HidingTabWidget::currentChanged, this, &ElementEditor::tabChanged);
    delete d;
}

void ElementEditor::apply()
{
    /// The prime problem to tackle in this function is to cope with
    /// invalid/problematic entry ids or macro keys, respectively:
    ///  - empty ids/keys
    ///  - ids/keys that are duplicates of already used ids/keys

    QSharedPointer<Entry> entry = d->element.dynamicCast<Entry>();
    QSharedPointer<Macro> macro = d->element.dynamicCast<Macro>();
    /// Determine id/key as it was set before the current editing started
    const QString originalId = !entry.isNull() ? entry->id() : (!macro.isNull() ? macro->key() : QString());
    /// Get the id/key as it is in the editing widget right now
    const QString newId = d->currentId();
    /// Keep track whether the 'original' id/key or the 'new' id/key will eventually be used
    enum IdToUse {UseOriginalId, UseNewId};
    IdToUse idToUse = UseNewId;

    if (newId.isEmpty() && !originalId.isEmpty()) {
        /// New id/key is empty (invalid by definition), so just notify use and revert back to original id/key
        /// (assuming that original id/key is valid)
        KMessageBox::sorry(this, i18n("No id was entered, so the previous id '%1' will be restored.", originalId), i18n("No id given"));
        idToUse = UseOriginalId;
    } else if (!newId.isEmpty()) {
        /// If new id/key is not empty, then check if it is identical to another entry/macro in the current file
        const QSharedPointer<Element> knownElementWithSameId = d->currentFile() != nullptr ? d->currentFile()->containsKey(newId) : QSharedPointer<Element>();
        if (!knownElementWithSameId.isNull() && d->element != knownElementWithSameId) {
            /// Some other, different element (entry or macro) uses same id/key, so ask user how to proceed
            const int msgBoxResult = KMessageBox::warningContinueCancel(this, i18n("The entered id '%1' is already in use for another element.\n\nKeep original id '%2' instead?", newId, originalId), i18n("Id already in use"), KGuiItem(i18n("Keep duplicate ids")), KGuiItem(i18n("Restore original id")));
            idToUse = msgBoxResult == KMessageBox::Continue ? UseNewId : UseOriginalId;
        }
    }

    if (idToUse == UseOriginalId) {
        /// As 'apply()' above set the 'new' id/key but the 'original' id/key is to be used,
        /// now UI must be updated accordingly. Changes will propagate to the entry id or
        /// macro key, respectively, when invoking apply() further down
        d->setCurrentId(originalId);
    }

    /// Apply will always set the 'new' id/key to the entry or macro, respectively
    d->apply();

    d->setModified(false);
    emit modified(false);
}

void ElementEditor::reset()
{
    d->reset();
    emit modified(false);
}

bool ElementEditor::validate() {
    QWidget *widgetWithIssue = nullptr;
    QString message;
    if (!validate(&widgetWithIssue, message)) {
        const QString msgBoxMessage = message.isEmpty() ? i18n("Validation for the current element failed.") : i18n("Validation for the current element failed:\n%1", message);
        KMessageBox::error(this, msgBoxMessage, i18n("Element validation failed"));
        if (widgetWithIssue != nullptr) {
            /// Probe if widget with issue is inside a QTabWiget; if yes, make parenting tab the current tab
            QWidget *cur = widgetWithIssue;
            do {
                QTabWidget *tabWidget = cur->parent() != nullptr && cur->parent()->parent() != nullptr ? qobject_cast<QTabWidget *>(cur->parent()->parent()) : nullptr;
                if (tabWidget != nullptr) {
                    tabWidget->setCurrentWidget(cur);
                    break;
                }
                cur = qobject_cast<QWidget *>(cur->parent());
            } while (cur != nullptr);
            /// Set focus to widget with issue
            widgetWithIssue->setFocus();
        }
        return false;
    }
    return true;
}

void ElementEditor::setElement(QSharedPointer<Element> element, const File *file)
{
    d->setElement(element, file);
    d->reset();
    emit modified(false);
}

void ElementEditor::setElement(QSharedPointer<const Element> element, const File *file)
{
    QSharedPointer<Element> clone;
    QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (!entry.isNull())
        clone = QSharedPointer<Entry>(new Entry(*entry.data()));
    else {
        QSharedPointer<const Macro> macro = element.dynamicCast<const Macro>();
        if (!macro.isNull())
            clone = QSharedPointer<Macro>(new Macro(*macro.data()));
        else {
            QSharedPointer<const Preamble> preamble = element.dynamicCast<const Preamble>();
            if (!preamble.isNull())
                clone = QSharedPointer<Preamble>(new Preamble(*preamble.data()));
            else {
                QSharedPointer<const Comment> comment = element.dynamicCast<const Comment>();
                if (!comment.isNull())
                    clone = QSharedPointer<Comment>(new Comment(*comment.data()));
                else
                    Q_ASSERT_X(element == nullptr, "ElementEditor::ElementEditor(const Element *element, QWidget *parent)", "element is not NULL but could not be cast on a valid Element sub-class");
            }
        }
    }

    d->setElement(clone, file);
    d->reset();
}

void ElementEditor::setReadOnly(bool isReadOnly)
{
    d->setReadOnly(isReadOnly);
}

bool ElementEditor::elementChanged()
{
    return d->elementChanged;
}

bool ElementEditor::elementUnapplied()
{
    return d->elementUnapplied;
}

bool ElementEditor::validate(QWidget **widgetWithIssue, QString &message)
{
    return d->validate(widgetWithIssue, message);
}

QWidget *ElementEditor::currentPage() const
{
    return d->tab->currentWidget();
}

void ElementEditor::setCurrentPage(QWidget *page)
{
    if (d->tab->indexOf(page) >= 0)
        d->tab->setCurrentWidget(page);
}

void ElementEditor::tabChanged()
{
    d->switchTo(d->tab->currentWidget());
}

void ElementEditor::checkBibTeX()
{
    d->checkBibTeX();
}

void ElementEditor::childModified(bool m)
{
    if (m) {
        d->elementUnapplied = true;
        d->referenceWidgetSetEntryIdByDefault();
    }
    emit modified(m);
}

void ElementEditor::updateReqOptWidgets()
{
    d->updateReqOptWidgets();
}

void ElementEditor::limitKeyboardTabStops()
{
    d->limitKeyboardTabStops();
}
