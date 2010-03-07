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

#include <QMenu>
#include <QSignalMapper>
#include <QBuffer>

#include <KDebug>
#include <KMessageBox>
#include <KGlobalSettings>
#include <KLocale>

#include <file.h>
#include <entry.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include "fieldlineedit.h"

using namespace KBibTeX::GUI::Widgets;

FieldLineEdit::FieldLineEdit(TypeFlags typeFlags, QWidget *parent)
        : MenuLineEdit(parent), m_typeFlags(typeFlags)
{
    m_menuTypes = new QMenu(i18n("Types"), this);
    setMenu(m_menuTypes);
    m_menuTypesSignalMapper = new QSignalMapper(this);
    connect(m_menuTypesSignalMapper, SIGNAL(mapped(int)), this, SLOT(slotTypeChanged(int)));

    setupMenu();

    setTypeFlag(Text);
}

FieldLineEdit::TypeFlag FieldLineEdit::typeFlag()
{
    return m_typeFlag;
}

FieldLineEdit::TypeFlag FieldLineEdit::setTypeFlag(FieldLineEdit::TypeFlag typeFlag)
{
    if (m_typeFlags&typeFlag) m_typeFlag = typeFlag;
    else if (m_typeFlags&Text) m_typeFlag = Text;
    else if (m_typeFlags&Reference) m_typeFlag = Reference;
    else if (m_typeFlags&Person) m_typeFlag = Person;
    else if (m_typeFlags&Keyword) m_typeFlag = Keyword;
    else if (m_typeFlags&Source) m_typeFlag = Source;
    else {
        kWarning() << "No valid TypeFlags given, resetting to Source";
        m_typeFlags = Source;
        m_typeFlag = Source;
        setupMenu();
    }

    updateGUI();

    return m_typeFlag;
}

FieldLineEdit::TypeFlag FieldLineEdit::setTypeFlags(TypeFlags typeFlags)
{
    m_typeFlags = typeFlags;
    setupMenu();
    return setTypeFlag(m_typeFlag);
}

void FieldLineEdit::setValue(const KBibTeX::IO::Value& value)
{
    m_originalValue = value;
    loadValue(m_originalValue);
}

void FieldLineEdit::applyTo(KBibTeX::IO::Value& value) const
{
    value.clear();
    switch (m_typeFlag) {
    case Text:
        value.append(new KBibTeX::IO::PlainText(text()));
        break;
    case Reference:
        value.append(new KBibTeX::IO::MacroKey(text())); // FIXME Check that text contains only valid characters!
        break;
    case Person:
        value.append(KBibTeX::IO::FileImporterBibTeX::splitName(text()));
        break;
    case Keyword: {
        QList<KBibTeX::IO::Keyword*> keywords = KBibTeX::IO::FileImporterBibTeX::splitKeywords(text());
        for (QList<KBibTeX::IO::Keyword*>::Iterator it = keywords.begin(); it != keywords.end(); ++it)
            value.append(*it);
    }
    break;
    case Source: {
        QString key = "title"; // FIXME "author" is only required for persons, use something else for plain text
        KBibTeX::IO::FileImporterBibTeX importer;
        QString fakeBibTeXFile = QString("@article{dummy, %1=%2}").arg(key).arg(text());
        kDebug() << "fakeBibTeXFile=" << fakeBibTeXFile << endl;
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        QTextStream ts(&buffer);
        ts << fakeBibTeXFile << endl;
        buffer.close();

        buffer.open(QIODevice::ReadOnly);
        KBibTeX::IO::File *file = importer.load(&buffer);
        KBibTeX::IO::Entry *entry = dynamic_cast< KBibTeX::IO::Entry*>(file->first());
        if (entry != NULL) {
            value = entry->value(key);
            kDebug() << "value->count()=" << value.count() << "  " << entry->value(key).count();
        } else
            kError() << "Cannot create value";
        delete file;
        buffer.close();
    }
    break;
    }
}

void FieldLineEdit::reset()
{
    loadValue(m_originalValue);
}

KIcon FieldLineEdit::iconForTypeFlag(TypeFlag typeFlag)
{
    switch (typeFlag) {
    case Text: return KIcon("draw-text");
    case Reference: return KIcon("emblem-symbolic-link");
    case Person: return KIcon("user-identity");
    case Keyword: return KIcon("edit-find");
    case Source: return KIcon("code-context");
    default: return KIcon();
    };
}


void FieldLineEdit::loadValue(const KBibTeX::IO::Value& value)
{
    QString text = "";
    m_incompleteRepresentation = false;

    if (value.size() > 0) {
        if (m_typeFlag == Source) {
            KBibTeX::IO::FileExporterBibTeX *exporter = new KBibTeX::IO::FileExporterBibTeX();
            text = exporter->valueToBibTeX(value);
            delete exporter;
        } else {
            m_incompleteRepresentation = value.size() > 1;
            const KBibTeX::IO::ValueItem *first = value.first();
            const KBibTeX::IO::PlainText *plainText = dynamic_cast<const KBibTeX::IO::PlainText*>(first);
            if (plainText != NULL) {
                text = plainText->text();
            } else {
                const KBibTeX::IO::Person *person = dynamic_cast<const KBibTeX::IO::Person*>(first);
                if (person != NULL) {
                    text = person->lastName();
                    QString temp = person->firstName();
                    if (!temp.isEmpty()) text.prepend(temp + " ");
                    temp = person->suffix();
                    if (!temp.isEmpty()) text.append(", " + temp);
                    temp = person->prefix();
                    if (!temp.isEmpty()) text.prepend(temp + " ");
                } else {
                    const KBibTeX::IO::MacroKey *macroKey = dynamic_cast<const KBibTeX::IO::MacroKey*>(first);
                    if (macroKey != NULL) {
                        text = macroKey->text();
                    }
                }
            }
        }
    }


    if (m_incompleteRepresentation)
        m_incompleteRepresentation = KMessageBox::warningContinueCancel(this, i18n("The chosen representation cannot display the value of this field.\n\nUsing this representation will cause lost of data."), i18n("Chosen representation"), KGuiItem(i18n("Continue with data loss"), "continue"), KGuiItem(i18n("Make field read-only"), "readonly")) != KMessageBox::Continue;
    setReadOnly(m_incompleteRepresentation); // FIXME: This may cause trouble with "Apply"
    setText(text);
}

void FieldLineEdit::setupMenu()
{
    m_menuTypes->clear();

    if (m_typeFlags&Text) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Text), i18n("Plain Text"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Text);
    }
    if (m_typeFlags&Reference) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Reference), i18n("Reference"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Reference);
    }
    if (m_typeFlags&Person) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Person), i18n("Person"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Person);
    }
    if (m_typeFlags&Source) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(Source), i18n("Source Code"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, Source);
    }
}

void FieldLineEdit::updateGUI()
{
    setFont(KGlobalSettings::generalFont());
    setIcon(iconForTypeFlag(m_typeFlag));
    switch (m_typeFlag) {
    case Text: setButtonToolTip(i18n("Plain Text")); break;
    case Reference: setButtonToolTip(i18n("Reference")); break;
    case Person: setButtonToolTip(i18n("Person")); break;
    case Source:
        setButtonToolTip(i18n("Source Code"));
        setFont(KGlobalSettings::fixedFont());
        break;
    default: setButtonToolTip(""); break;
    };
}

void FieldLineEdit::slotTypeChanged(int newTypeFlag)
{
    KBibTeX::IO::Value value = m_originalValue;
    if (!m_incompleteRepresentation)
        applyTo(value);

    m_typeFlag = (TypeFlag)newTypeFlag;
    updateGUI();

    loadValue(value);
}
