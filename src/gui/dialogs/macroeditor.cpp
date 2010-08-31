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

#include <QWidget>
#include <QTextEdit>
#include <QLayout>
#include <QBuffer>
#include <QTextStream>
#include <QLabel>

#include <KPushButton>
#include <KGlobalSettings>
#include <KLocale>
#include <KDebug>
#include <KLineEdit>

#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <macro.h>
#include <fieldlineedit.h>
#include "macroeditor.h"

class MacroEditorSource : public QWidget
{
private:
    Macro *macro;
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
        sourceEdit->setTabStopWidth(QFontMetrics(sourceEdit->font()).averageCharWidth() * 4);

        KPushButton *buttonRestore = new KPushButton(i18n("Restore"), this);
        layout->addWidget(buttonRestore, 1, 1, 1, 1);
        connect(buttonRestore, SIGNAL(clicked()), parent(), SLOT(reset()));
    }

public:
    MacroEditorSource(QWidget *parent)
            : QWidget(parent) {
        createGUI();
    }

    bool apply(Macro *macro) {
        bool result = false;
        QString text = sourceEdit->document()->toPlainText();
        FileImporterBibTeX importer;
        File *file = importer.fromString(text);
        if (file == NULL) return false;

        if (file->count() == 1) {
            Macro *readMacro = dynamic_cast<Macro*>(file->first());
            if (readMacro != NULL) {
                macro->operator =(*readMacro);
                result = true;
            }
        }

        delete file;
        return result;
    }

    void reset(const Macro *macro) {
        FileExporterBibTeX exporter;
        QBuffer textBuffer;
        textBuffer.open(QIODevice::WriteOnly);
        exporter.save(&textBuffer, macro, NULL);
        textBuffer.close();
        textBuffer.open(QIODevice::ReadOnly);
        QTextStream ts(&textBuffer);
        QString text = ts.readAll();
        sourceEdit->document()->setPlainText(text);
    }

    void apply() {
        apply(macro);
    }

    void reset() {
        reset(macro);
    }

    void setReadOnly(bool isReadOnly) {
        Q_UNUSED(isReadOnly);
        // TODO
    }
};

class MacroEditorGUI : public QWidget
{
private:
    Macro *macro;

    void createGUI() {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *label = new QLabel(i18n("Key:"), this);
        layout->addWidget(label, 0);
        lineEditKey = new KLineEdit(this);
        layout->addWidget(lineEditKey, 0);
        label->setBuddy(lineEditKey);

        label = new QLabel(i18n("Value:"), this);
        layout->addWidget(label, 0);
        lineEditValue = new FieldLineEdit(KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfSource, true);
        layout->addWidget(lineEditValue, 1);
        label->setBuddy(lineEditValue);
    }

public:
    KLineEdit *lineEditKey;
    FieldLineEdit *lineEditValue;

    MacroEditorGUI(QWidget *parent)
            : QWidget(parent) {
        createGUI();
    }

    void apply(Macro *macro) {
        macro->setKey(lineEditKey->text());
        Value value;
        lineEditValue->apply(value);
        macro->setValue(value);
    }

    void reset(const Macro *macro) {
        lineEditKey->setText(macro->key());
        lineEditValue->reset(macro->value());
    }

    void apply() {
        apply(macro);
    }

    void reset() {
        reset(macro);
    }

    void setReadOnly(bool isReadOnly) {
        Q_UNUSED(isReadOnly);
        // TODO
    }
};

class MacroEditor::MacroEditorPrivate
{
private:
    MacroEditorGUI *tabGlobal;
    MacroEditorSource *tabSource;

public:
    Macro *macro;
    MacroEditor *p;
    bool isModified;

    MacroEditorPrivate(Macro *m, MacroEditor *parent)
            : macro(m), p(parent) {
        isModified = false;
    }

    void createGUI() {
        tabGlobal = new MacroEditorGUI(p);
        p->addTab(tabGlobal, i18n("Global"));
        tabSource = new MacroEditorSource(p);
        p->addTab(tabSource, i18n("Source"));
    }

    void apply() {
        apply(macro);
    }

    void apply(Macro *macro) {
        if (p->currentWidget() == tabGlobal)
            tabGlobal->apply(macro);
        else  if (p->currentWidget() == tabSource)
            tabSource->apply(macro);
    }

    void reset() {
        reset(macro);
    }

    void reset(const Macro *macro) {
        tabGlobal->reset(macro);
        tabSource->reset(macro);
    }

    void setReadOnly(bool isReadOnly) {
        tabGlobal->setReadOnly(isReadOnly);
        tabSource->setReadOnly(isReadOnly);
    }

    void switchTo(QWidget *newTab) {
        if (newTab == tabGlobal) {
            Macro temp;
            tabSource->apply(&temp);
            tabGlobal->reset(&temp);
        } else if (newTab == tabSource) {
            Macro temp;
            tabGlobal->apply(&temp);
            tabSource->reset(&temp);
        }
    }
};

MacroEditor::MacroEditor(Macro *macro, QWidget *parent)
        : QTabWidget(parent), d(new MacroEditorPrivate(macro, this))
{
    kDebug() << "Editing " << macro->key() << " = " << macro->value();
    d->createGUI();
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));
}

MacroEditor::MacroEditor(const Macro *macro, QWidget *parent)
        : QTabWidget(parent)
{
    kDebug() << "Editing " << macro->key() << " = " << macro->value();
    Macro *m = new Macro(*macro);
    d = new MacroEditorPrivate(m, this);
    d->createGUI();
    setReadOnly(true);
}

void MacroEditor::apply()
{
    d->apply();
}

void MacroEditor::reset()
{
    d->reset();
    d->isModified = false;
}

void MacroEditor::setReadOnly(bool isReadOnly)
{
    d->setReadOnly(isReadOnly);
}

void MacroEditor::tabChanged()
{
    d->switchTo(currentWidget());
}
