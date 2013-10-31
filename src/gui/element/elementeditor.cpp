/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QBuffer>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>
#include <QMenu>
#include <QScrollArea>

#include <KPushButton>
#include <KMessageBox>
#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>
#include <kio/netaccess.h>

#include "menulineedit.h"
#include "entry.h"
#include "comment.h"
#include "macro.h"
#include "preamble.h"
#include "element.h"
#include "file.h"
#include "elementwidgets.h"
#include "elementeditor.h"
#include "checkbibtex.h"
#include "hidingtabwidget.h"

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
    ElementWidget *sourceWidget;
    KPushButton *buttonCheckWithBibTeX;

    /// Settings management through a push button with menu
    KSharedConfigPtr config;
    KPushButton *buttonOptions;
    QAction *actionForceShowAllWidgets, *actionLimitKeyboardTabStops;

public:
    QSharedPointer<Element> element;
    HidingTabWidget *tab;
    bool elementChanged, elementUnapplied;

    ElementEditorPrivate(bool scrollable, ElementEditor *parent)
            : file(NULL), p(parent), previousWidget(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), elementChanged(false), elementUnapplied(false) {
        createGUI(scrollable);
    }

    ~ElementEditorPrivate() {
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
        EntryLayout *el = EntryLayout::self();
        for (EntryLayout::ConstIterator elit = el->constBegin(); elit != el->constEnd(); ++elit) {
            QSharedPointer<EntryTabLayout> etl = *elit;
            ElementWidget *widget = new EntryConfiguredWidget(etl, tab);
            connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
            widgets << widget;
            if (previousWidget == NULL)
                previousWidget = widget; ///< memorize the first tab
            int index = tab->addTab(widget, widget->icon(), widget->label());
            tab->hideTab(index);
        }

        ElementWidget *widget = new PreambleWidget(tab);
        connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
        widgets << widget;
        int index = tab->addTab(widget, widget->icon(), widget->label());
        tab->hideTab(index);

        widget = new MacroWidget(tab);
        connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
        widgets << widget;
        index = tab->addTab(widget, widget->icon(), widget->label());
        tab->hideTab(index);

        FilesWidget *filesWidget = new FilesWidget(tab);
        filesWidget->setApplyElementInterface(this);
        connect(filesWidget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
        widgets << filesWidget;
        index = tab->addTab(filesWidget, filesWidget->icon(), filesWidget->label());
        tab->hideTab(index);

        QStringList blacklistedFields;

        /// blacklist fields covered by EntryConfiguredWidget
        for (EntryLayout::ConstIterator elit = el->constBegin(); elit != el->constEnd(); ++elit)
            for (QList<SingleFieldLayout>::ConstIterator sflit = (*elit)->singleFieldLayouts.constBegin(); sflit != (*elit)->singleFieldLayouts.constEnd(); ++sflit)
                blacklistedFields << (*sflit).bibtexLabel;

        /// blacklist fields covered by FilesWidget
        blacklistedFields << QString(Entry::ftUrl) << QString(Entry::ftLocalFile) << QString(Entry::ftDOI) << QLatin1String("ee") << QLatin1String("biburl") << QLatin1String("postscript");
        for (int i = 2; i < 256; ++i) // FIXME replace number by constant
            blacklistedFields << QString(Entry::ftUrl) + QString::number(i) << QString(Entry::ftLocalFile) + QString::number(i) <<  QString(Entry::ftDOI) + QString::number(i) << QLatin1String("ee") + QString::number(i) << QLatin1String("postscript") + QString::number(i);

        widget = new OtherFieldsWidget(blacklistedFields, tab);
        connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
        widgets << widget;
        index = tab->addTab(widget, widget->icon(), widget->label());
        tab->hideTab(index);

        sourceWidget = new SourceWidget(tab);
        connect(sourceWidget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
        widgets << sourceWidget;
        index = tab->addTab(sourceWidget, sourceWidget->icon(), sourceWidget->label());
        tab->hideTab(index);
    }

    void createGUI(bool scrollable) {
        /// load configuration for options push button
        static const QString configGroupName = QLatin1String("User Interface");
        static const QString keyEnableAllWidgets = QLatin1String("EnableAllWidgets");
        KConfigGroup configGroup(config, configGroupName);
        const bool showAll = configGroup.readEntry(keyEnableAllWidgets, true);
        const bool limitKeyboardTabStops = configGroup.readEntry(MenuLineEdit::keyLimitKeyboardTabStops, false);

        QBoxLayout *vLayout = new QVBoxLayout(p);

        referenceWidget = new ReferenceWidget(p);
        referenceWidget->setApplyElementInterface(this);
        connect(referenceWidget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
        connect(referenceWidget, SIGNAL(entryTypeChanged()), p, SLOT(updateReqOptWidgets()));
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
        buttonOptions = new KPushButton(KIcon("configure"), i18n("Options"), p);
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

        buttonCheckWithBibTeX = new KPushButton(KIcon("tools-check-spelling"), i18n("Check with BibTeX"), p);
        hLayout->addWidget(buttonCheckWithBibTeX, 0);
        connect(buttonCheckWithBibTeX, SIGNAL(clicked()), p, SLOT(checkBibTeX()));

        addTabWidgets();
    }

    void updateTabVisibility() {
        disconnect(tab, SIGNAL(currentChanged(int)), p, SLOT(tabChanged()));
        if (element.isNull()) {
            p->setEnabled(false);
        } else {
            p->setEnabled(true);
            int firstEnabledTab = 1024;

            for (WidgetList::ConstIterator it = widgets.constBegin(); it != widgets.constEnd(); ++it) {
                ElementWidget *widget = *it;
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
        connect(tab, SIGNAL(currentChanged(int)), p, SLOT(tabChanged()));
    }

    bool hasDuplicateId() const {
        return referenceWidget != NULL && referenceWidget->isDuplicateId();
    }

    void apply() {
        elementChanged = true;
        elementUnapplied = false;
        apply(element);
    }

    void apply(QSharedPointer<Element> element) {
        if (referenceWidget != NULL)
            referenceWidget->apply(element);
        ElementWidget *currentElementWidget = dynamic_cast<ElementWidget *>(tab->currentWidget());
        //Q_ASSERT_X(currentElementWidget != NULL || tab->currentWidget() == NULL, "ElementEditor::ElementEditorPrivate::apply", "Could not cast currentWidget to ElementWidget");
        for (WidgetList::ConstIterator it = widgets.constBegin(); it != widgets.constEnd(); ++it)
            if ((*it) != currentElementWidget && (*it) != sourceWidget)
                (*it)->apply(element);
        if (currentElementWidget != NULL)
            currentElementWidget->apply(element);
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

        internalEntry = QSharedPointer<Entry>();
        internalMacro = QSharedPointer<Macro>();
        internalComment = QSharedPointer<Comment>();
        internalPreamble = QSharedPointer<Preamble>();
        QSharedPointer<const Entry> e = element.dynamicCast<const Entry>();
        if (!e.isNull())
            internalEntry = QSharedPointer<Entry>(new Entry(*e.data()));
        else {
            QSharedPointer<const Macro> m = element.dynamicCast<const Macro>();
            if (!m.isNull())
                internalMacro = QSharedPointer<Macro>(new Macro(*m.data()));
            else {
                QSharedPointer<const Comment> c = element.dynamicCast<const Comment>();
                if (!c.isNull())
                    internalComment = QSharedPointer<Comment>(new Comment(*c.data()));
                else {
                    QSharedPointer<const Preamble> p = element.dynamicCast<const Preamble>();
                    if (!p.isNull())
                        internalPreamble = QSharedPointer<Preamble>(new Preamble(*p.data()));
                    else
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
        foreach(ElementWidget *elementWidget, widgets) {
            elementWidget->showReqOptWidgets(forceVisible, tempEntry->type());
        }

        /// save configuration
        static const QString configGroupName = QLatin1String("User Interface");
        static const QString keyEnableAllWidgets = QLatin1String("EnableAllWidgets");
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(keyEnableAllWidgets, actionForceShowAllWidgets->isChecked());
        config->sync();
    }

    void limitKeyboardTabStops() {
        /// save configuration
        static const QString configGroupName = QLatin1String("User Interface");
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(MenuLineEdit::keyLimitKeyboardTabStops, actionLimitKeyboardTabStops->isChecked());
        config->sync();

        /// notify all listening MenuLineEdit widgets to change their behavior
        NotificationHub::publishEvent(MenuLineEdit::MenuLineConfigurationChangedEvent);
    }

    void switchTo(QWidget *newTab) {
        bool isSourceWidget = newTab == sourceWidget;
        ElementWidget *newWidget = dynamic_cast<ElementWidget *>(newTab);
        if (previousWidget != NULL && newWidget != NULL) {
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

            previousWidget->apply(temp);
            if (isSourceWidget && referenceWidget != NULL) referenceWidget->apply(temp);
            newWidget->reset(temp);
            if (referenceWidget != NULL && dynamic_cast<SourceWidget *>(previousWidget) != NULL)
                referenceWidget->reset(temp);
        }
        previousWidget = newWidget;

        for (WidgetList::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->setEnabled(!isSourceWidget || *it == newTab);
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
    connect(d->tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
}

ElementEditor::~ElementEditor()
{
    disconnect(d->tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
    delete d;
}

void ElementEditor::apply()
{
    QSharedPointer<Entry> entry = d->element.dynamicCast<Entry>();
    QSharedPointer<Macro> macro = d->element.dynamicCast<Macro>();
    const bool doReplaceId = d->hasDuplicateId() && (
                                 /// the id of the entry is already in use by another entry
                                 (!entry.isNull() && KMessageBox::warningContinueCancel(this, i18n("The entered id '%1' is already in use for another entry.", entry->id()), i18n("Id already in use"), KGuiItem(i18n("Keep duplicate ids")), KGuiItem(i18n("Restore original id"))) == KMessageBox::Cancel)
                                 /// the key of a macro is already in use by another macro
                                 || (!macro.isNull() && KMessageBox::warningContinueCancel(this, i18n("The entered key '%1' is already in use for another macro.", macro->key()), i18n("Key already in use"), KGuiItem(i18n("Keep duplicate keys")), KGuiItem(i18n("Restore original key"))) == KMessageBox::Cancel)
                             );

    QString replacementId;
    if (doReplaceId) {
        if (!entry.isNull())
            replacementId = entry->id();
        else if (!macro.isNull())
            replacementId = macro->key();
    }

    d->apply();

    if (doReplaceId) {
        if (!entry.isNull())
            entry->setId(replacementId);
        else if (!macro.isNull())
            macro->setKey(replacementId);
        d->reset();
    }

    d->setModified(false);
    emit modified(false);
}

void ElementEditor::reset()
{
    d->reset();
    emit modified(false);
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
                    Q_ASSERT_X(element == NULL, "ElementEditor::ElementEditor(const Element *element, QWidget *parent)", "element is not NULL but could not be cast on a valid Element sub-class");
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
