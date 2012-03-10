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

#include <QCheckBox>
#include <QTabWidget>
#include <QLabel>
#include <QLayout>
#include <QBuffer>
#include <QTextStream>
#include <QApplication>
#include <QFileInfo>

#include <KPushButton>
#include <KMessageBox>
#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>
#include <kio/netaccess.h>

#include <entry.h>
#include <comment.h>
#include <macro.h>
#include <preamble.h>
#include <element.h>
#include <file.h>
#include <fileexporterblg.h>
#include "elementwidgets.h"
#include "elementeditor.h"

class ElementEditor::ElementEditorPrivate : public ElementEditor::ApplyElementInterface
{
private:
    QList<ElementWidget*> widgets;
    QSharedPointer<Element> element;
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
    QCheckBox *checkBoxForceShowAllWidgets;
    KSharedConfigPtr config;

public:
    QTabWidget *tab;
    bool elementChanged, elementUnapplied;

    ElementEditorPrivate(QSharedPointer<Element> m, const File *f, ElementEditor *parent)
            : element(m), file(f), p(parent), previousWidget(NULL), referenceWidget(NULL), sourceWidget(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), elementChanged(false), elementUnapplied(false) {
        createGUI();
    }

    void createGUI() {
        widgets.clear();
        EntryLayout *el = EntryLayout::self();

        QGridLayout *layout = new QGridLayout(p);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);

        if (ReferenceWidget::canEdit(element.data())) {
            referenceWidget = new ReferenceWidget(p);
            referenceWidget->setApplyElementInterface(this);
            connect(referenceWidget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
            layout->addWidget(referenceWidget, 0, 0, 1, 3);
            widgets << referenceWidget;
        } else
            referenceWidget = NULL;

        tab = new QTabWidget(p);
        layout->addWidget(tab, 1, 0, 1, 3);

        /// load configuration
        const QString configGroupName = QLatin1String("User Interface");
        const QString keyEnableAllWidgets = QLatin1String("EnableAllWidgets");
        KConfigGroup configGroup(config, configGroupName);
        const bool showAll = configGroup.readEntry(keyEnableAllWidgets, true);

        checkBoxForceShowAllWidgets = new QCheckBox(i18n("Show all fields"), p);
        checkBoxForceShowAllWidgets->setChecked(showAll);
        layout->addWidget(checkBoxForceShowAllWidgets, 2, 0, 1, 2);
        connect(checkBoxForceShowAllWidgets, SIGNAL(toggled(bool)), p, SLOT(updateReqOptWidgets()));
        if (referenceWidget != NULL)
            connect(referenceWidget, SIGNAL(entryTypeChanged()), p, SLOT(updateReqOptWidgets()));

        buttonCheckWithBibTeX = new KPushButton(KIcon("tools-check-spelling"), i18n("Check with BibTeX"), p);
        layout->addWidget(buttonCheckWithBibTeX, 2, 2, 1, 1);
        connect(buttonCheckWithBibTeX, SIGNAL(clicked()), p, SLOT(checkBibTeX()));

        if (EntryConfiguredWidget::canEdit(element.data()))
            for (EntryLayout::ConstIterator elit = el->constBegin(); elit != el->constEnd(); ++elit) {
                EntryTabLayout etl = *elit;
                ElementWidget *widget = new EntryConfiguredWidget(etl, tab);
                connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
                tab->addTab(widget, widget->icon(), widget->label());
                widgets << widget;
            }

        if (PreambleWidget::canEdit(element.data())) {
            ElementWidget *widget = new PreambleWidget(tab);
            connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
            tab->addTab(widget, widget->icon(), widget->label());
            widgets << widget;
        }

        if (MacroWidget::canEdit(element.data())) {
            ElementWidget *widget = new MacroWidget(tab);
            connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
            tab->addTab(widget, widget->icon(), widget->label());
            widgets << widget;
        }

        if (FilesWidget::canEdit(element.data())) {
            ElementWidget *widget = new FilesWidget(tab);
            connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
            tab->addTab(widget, widget->icon(), widget->label());
            widgets << widget;
        }

        if (OtherFieldsWidget::canEdit(element.data())) {
            QStringList blacklistedFields;
            /// blacklist fields covered by EntryConfiguredWidget
            for (EntryLayout::ConstIterator elit = el->constBegin(); elit != el->constEnd(); ++elit)
                for (QList<SingleFieldLayout>::ConstIterator sflit = (*elit).singleFieldLayouts.constBegin(); sflit != (*elit).singleFieldLayouts.constEnd();++sflit)
                    blacklistedFields << (*sflit).bibtexLabel;

            /// blacklist fields covered by FilesWidget
            blacklistedFields << QString(Entry::ftUrl) << QString(Entry::ftLocalFile) << QString(Entry::ftDOI) << QLatin1String("ee") << QLatin1String("biburl") << QLatin1String("postscript");
            for (int i = 2; i < 256; ++i) // FIXME replace number by constant
                blacklistedFields << QString(Entry::ftUrl) + QString::number(i) << QString(Entry::ftLocalFile) + QString::number(i) <<  QString(Entry::ftDOI) + QString::number(i) << QLatin1String("ee") + QString::number(i) << QLatin1String("postscript") + QString::number(i);

            ElementWidget *widget = new OtherFieldsWidget(blacklistedFields, tab);
            connect(widget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
            tab->addTab(widget, widget->icon(), widget->label());
            widgets << widget;
        }

        if (SourceWidget::canEdit(element.data())) {
            sourceWidget = new SourceWidget(tab);
            connect(sourceWidget, SIGNAL(modified(bool)), p, SLOT(childModified(bool)));
            tab->addTab(sourceWidget, sourceWidget->icon(), sourceWidget->label());
            widgets << sourceWidget;
        }

        previousWidget = dynamic_cast<ElementWidget*>(tab->widget(0));
    }

    void apply() {
        elementChanged = true;
        elementUnapplied = false;
        apply(element);
    }

    virtual void apply(QSharedPointer<Element> element) {
        if (referenceWidget != NULL)
            referenceWidget->apply(element);
        ElementWidget *currentElementWidget = dynamic_cast<ElementWidget*>(tab->currentWidget());
        for (QList<ElementWidget*>::ConstIterator it = widgets.constBegin(); it != widgets.constEnd(); ++it)
            if ((*it) != currentElementWidget && (*it) != sourceWidget)
                (*it)->apply(element);
        currentElementWidget->apply(element);
    }

    void reset() {
        elementChanged = false;
        elementUnapplied = false;
        reset(element);

        /// show checkbox to enable all fields only if editing an entry
        checkBoxForceShowAllWidgets->setVisible(!internalEntry.isNull());
        /// Disable widgets if necessary
        if (!checkBoxForceShowAllWidgets->isChecked())
            updateReqOptWidgets();
    }

    void reset(QSharedPointer<const Element> element) {
        for (QList<ElementWidget*>::Iterator it = widgets.begin(); it != widgets.end(); ++it) {
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
                        Q_ASSERT_X(element.isNull(), "ElementEditor::ElementEditorPrivate::reset(const Element *element)", "element is not NULL but could not be cast on a valid Element sub-class");
                }
            }
        }

        buttonCheckWithBibTeX->setVisible(typeid(*element) == typeid(Entry));
    }

    void setReadOnly(bool isReadOnly) {
        for (QList<ElementWidget*>::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->setReadOnly(isReadOnly);
    }

    void updateReqOptWidgets() {
        /// this function is only relevant if editing an entry (and not e.g. a comment)
        if (internalEntry.isNull()) return; /// quick-and-dirty test if editing an entry

        /// make a temporary snapshot of the current state
        QSharedPointer<Entry> tempEntry = QSharedPointer<Entry>(new Entry());
        apply(tempEntry);

        /// update the enabled/disabled state of required and optional widgets/fields
        bool forceVisible = checkBoxForceShowAllWidgets->isChecked();
        foreach(ElementWidget *elementWidget, widgets) {
            elementWidget->showReqOptWidgets(forceVisible, tempEntry->type());
        }

        /// save configuration
        QString const configGroupName = QLatin1String("User Interface");
        QString const keyEnableAllWidgets = QLatin1String("EnableAllWidgets");
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(keyEnableAllWidgets, checkBoxForceShowAllWidgets->isChecked());
        config->sync();
    }

    void switchTo(QWidget *newTab) {
        bool isSourceWidget = newTab == sourceWidget;
        ElementWidget *newWidget = dynamic_cast<ElementWidget*>(newTab);
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
            Q_ASSERT(!temp.isNull());

            previousWidget->apply(temp);
            if (isSourceWidget && referenceWidget != NULL) referenceWidget->apply(temp);
            newWidget->reset(temp);
            if (referenceWidget != NULL && dynamic_cast<SourceWidget*>(previousWidget) != NULL)
                referenceWidget->reset(temp);
        }
        previousWidget = newWidget;

        for (QList<ElementWidget*>::Iterator it = widgets.begin();it != widgets.end();++it)
            (*it)->setEnabled(!isSourceWidget || *it == newTab);
    }

    /**
      * Test current entry if it compiles with BibTeX.
      * Show warnings and errors in message box.
      */
    void checkBibTeX() {
        if (typeid(*element) == typeid(Entry)) {
            /// only entries are supported, no macros, preambles, ...

            /// disable GUI under process
            p->setEnabled(false);
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            /// use a dummy BibTeX file to collect all elements necessary for check
            File dummyFile;

            /// create temporary entry to work with
            QSharedPointer<Entry> entry = QSharedPointer<Entry>(new Entry(*internalEntry.data()));
            apply(entry);
            dummyFile << entry;

            /// fetch and inser crossref'ed entry
            QString crossRefStr = QString::null;
            Value crossRefVal = entry->value(Entry::ftCrossRef);
            if (!crossRefVal.isEmpty() && file != NULL) {
                crossRefStr = PlainTextValue::text(crossRefVal, file);
                QSharedPointer<Entry> crossRefDest = file->containsKey(crossRefStr, File::etEntry).dynamicCast<Entry>();
                if (!crossRefDest.isNull())
                    dummyFile << crossRefDest;
                else
                    crossRefStr = QString::null; /// memorize crossref'ing failed
            }

            /// include all macro definitions, in case they are referenced
            if (file != NULL)
                for (File::ConstIterator it = file->constBegin(); it != file->constEnd(); ++it)
                    if (typeid(**it) == typeid(Macro))
                        dummyFile << *it;

            /// run special exporter to get BibTeX's ouput
            QStringList bibtexOuput;
            QByteArray ba;
            QBuffer buffer(&ba);
            buffer.open(QIODevice::WriteOnly);
            FileExporterBLG exporter;
            bool result = exporter.save(&buffer, &dummyFile, &bibtexOuput);
            buffer.close();

            if (!result) {
                QApplication::restoreOverrideCursor();
                KMessageBox::errorList(p, i18n("Running BibTeX failed.\n\nSee the following output to trace the error."), bibtexOuput);
                p->setEnabled(true);
                return;
            }

            /// define variables how to parse BibTeX's ouput
            static const QString warningStart = QLatin1String("Warning--");
            static const QRegExp warningEmptyField("empty (\\w+) in ");
            static const QRegExp warningEmptyField2("empty (\\w+) or (\\w+) in ");
            static const QRegExp warningThereIsBut("there's a (\\w+) but no (\\w+) in");
            static const QRegExp warningCantUseBoth("can't use both (\\w+) and (\\w+) fields");
            static const QRegExp warningSort2("to sort, need (\\w+) or (\\w+) in ");
            static const QRegExp warningSort3("to sort, need (\\w+), (\\w+), or (\\w+) in ");
            static const QRegExp errorLine("---line (\\d+)");

            /// go line-by-line through BibTeX output and collect warnings/errors
            QStringList warnings;
            QString errorPlainText;
            for (QStringList::ConstIterator it = bibtexOuput.constBegin(); it != bibtexOuput.constEnd();++it) {
                QString line = *it;

                if (errorLine.indexIn(line) > -1) {
                    buffer.open(QIODevice::ReadOnly);
                    QTextStream ts(&buffer);
                    for (int i = errorLine.cap(1).toInt();i > 1;--i) {
                        errorPlainText = ts.readLine();
                        buffer.close();
                    }
                } else if (line.startsWith(QLatin1String("Warning--"))) {
                    /// is a warning ...

                    if (warningEmptyField.indexIn(line) > -1) {
                        /// empty/missing field
                        warnings << i18n("Field <b>%1</b> is empty", warningEmptyField.cap(1));
                    } else if (warningEmptyField2.indexIn(line) > -1) {
                        /// two empty/missing fields
                        warnings << i18n("Field <b>%1</b> and <b>%2</b> is empty, need at least one", warningEmptyField2.cap(1), warningEmptyField2.cap(2));
                    } else if (warningThereIsBut.indexIn(line) > -1) {
                        /// there is a field which exists but another does not exist
                        warnings << i18n("Field <b>%1</b> exists, but <b>%2</b> does not exist", warningThereIsBut.cap(1), warningThereIsBut.cap(2));
                    } else if (warningCantUseBoth.indexIn(line) > -1) {
                        /// there are two conflicting fields, only one may be used
                        warnings << i18n("Fields <b>%1</b> and <b>%2</b> cannot be used at the same time", warningCantUseBoth.cap(1), warningCantUseBoth.cap(2));
                    } else if (warningSort2.indexIn(line) > -1) {
                        /// one out of two fields missing for sorting
                        warnings << i18n("Fields <b>%1</b> or <b>%2</b> are required to sort entry", warningSort2.cap(1), warningSort2.cap(2));
                    } else if (warningSort3.indexIn(line) > -1) {
                        /// one out of three fields missing for sorting
                        warnings << i18n("Fields <b>%1</b>, <b>%2</b>, <b>%3</b> are required to sort entry", warningSort3.cap(1), warningSort3.cap(2), warningSort3.cap(3));
                    } else {
                        /// generic/unknown warning
                        line = line.mid(warningStart.length());
                        warnings << i18n("Unknown warning: %1", line);
                    }
                }
            }

            QApplication::restoreOverrideCursor();
            if (!errorPlainText.isEmpty())
                KMessageBox::information(p, i18n("<qt><p>The following error was found:</p><pre>%1</pre>", errorPlainText));
            else if (!warnings.isEmpty())
                KMessageBox::information(p, i18n("<qt><p>The following warnings were found:</p><ul><li>%1</li></ul>", warnings.join("</li><li>")));
            else
                KMessageBox::information(p, i18n("No warnings or errors were found.%1", crossRefStr.isNull() ? QLatin1String("") : i18n("\n\nSome fields missing in this entry where taken from the crossref'ed entry \"%1\".", crossRefStr)));

            p->setEnabled(true);
        }

    }

    void setModified(bool newIsModified) {
        for (QList<ElementWidget*>::Iterator it = widgets.begin(); it != widgets.end(); ++it)
            (*it)->setModified(newIsModified);
    }

};

ElementEditor::ElementEditor(QSharedPointer<Element> element, const File *file, QWidget *parent)
        : QWidget(parent), d(new ElementEditorPrivate(element, file, this))
{
    connect(d->tab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
    d->reset();
}

ElementEditor::ElementEditor(QSharedPointer<const Element> element, const File *file, QWidget *parent)
        : QWidget(parent)
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

    d = new ElementEditorPrivate(clone, file, this);
    setReadOnly(true);
}

ElementEditor::~ElementEditor()
{
    delete d;
}

void ElementEditor::apply()
{
    d->apply();
    d->setModified(false);
    emit modified(false);
}

void ElementEditor::reset()
{
    d->reset();
    emit modified(false);
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

int ElementEditor::currentTab()
{
    return d->tab->currentIndex();
}

void ElementEditor::setCurrentTab(int tabIndex)
{
    d->tab->setCurrentIndex(tabIndex);
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
    if (m)
        d->elementUnapplied = true;
    emit modified(m);
}

void ElementEditor::updateReqOptWidgets()
{
    d->updateReqOptWidgets();
}
