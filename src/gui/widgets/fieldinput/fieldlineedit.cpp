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

FieldLineEdit::FieldLineEdit(KBibTeX::TypeFlags typeFlags, bool isMultiLine, QWidget *parent)
        : MenuLineEdit(isMultiLine, parent), m_typeFlags(typeFlags)
{
    setObjectName(QLatin1String("FieldLineEdit"));
    m_menuTypes = new QMenu(i18n("Types"), this);
    setMenu(m_menuTypes);
    m_menuTypesSignalMapper = new QSignalMapper(this);
    connect(m_menuTypesSignalMapper, SIGNAL(mapped(int)), this, SLOT(slotTypeChanged(int)));

    setupMenu();

    setTypeFlag(KBibTeX::tfText);
}

KBibTeX::TypeFlag FieldLineEdit::typeFlag()
{
    return m_typeFlag;
}

KBibTeX::TypeFlag FieldLineEdit::setTypeFlag(KBibTeX::TypeFlag typeFlag)
{
    if (m_typeFlags&typeFlag) m_typeFlag = typeFlag;
    else if (m_typeFlags&KBibTeX::tfText) m_typeFlag = KBibTeX::tfText;
    else if (m_typeFlags&KBibTeX::tfReference) m_typeFlag = KBibTeX::tfReference;
    else if (m_typeFlags&KBibTeX::tfPerson) m_typeFlag = KBibTeX::tfPerson;
    else if (m_typeFlags&KBibTeX::tfKeyword) m_typeFlag = KBibTeX::tfKeyword;
    else if (m_typeFlags&KBibTeX::tfSource) m_typeFlag = KBibTeX::tfSource;
    else {
        kWarning() << "No valid TypeFlags given, resetting to Source";
        m_typeFlags = KBibTeX::tfSource;
        m_typeFlag = KBibTeX::tfSource;
        setupMenu();
    }

    updateGUI();

    return m_typeFlag;
}

KBibTeX::TypeFlag FieldLineEdit::setTypeFlags(KBibTeX::TypeFlags typeFlags)
{
    m_typeFlags = typeFlags;
    setupMenu();
    return setTypeFlag(m_typeFlag);
}

void FieldLineEdit::setValue(const Value& value)
{
    m_originalValue = value;
    loadValue(m_originalValue);
}

void FieldLineEdit::applyTo(Value& value) const
{
    value.clear();
    switch (m_typeFlag) {
    case KBibTeX::tfText:
        value.append(new PlainText(text()));
        break;
    case KBibTeX::tfReference:
        value.append(new MacroKey(text())); // FIXME Check that text contains only valid characters!
        break;
    case KBibTeX::tfPerson:
        value.append(FileImporterBibTeX::splitName(text()));
        break;
    case KBibTeX::tfKeyword: {
        QList<Keyword*> keywords = FileImporterBibTeX::splitKeywords(text());
        for (QList<Keyword*>::Iterator it = keywords.begin(); it != keywords.end(); ++it)
            value.append(*it);
    }
    break;
    case KBibTeX::tfSource:
        if (!text().isEmpty()) {
            QString key = "title"; // FIXME "author" is only required for persons, use something else for plain text
            FileImporterBibTeX importer;
            QString fakeBibTeXFile = QString("@article{dummy, %1=%2}").arg(key).arg(text());
            kDebug() << "fakeBibTeXFile=" << fakeBibTeXFile << endl;
            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly);
            QTextStream ts(&buffer);
            ts << fakeBibTeXFile << endl;
            buffer.close();

            buffer.open(QIODevice::ReadOnly);
            File *file = importer.load(&buffer);
            Entry *entry = dynamic_cast< Entry*>(file->first());
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

KIcon FieldLineEdit::iconForTypeFlag(KBibTeX::TypeFlag typeFlag)
{
    switch (typeFlag) {
    case KBibTeX::tfText: return KIcon("draw-text");
    case KBibTeX::tfReference: return KIcon("emblem-symbolic-link");
    case KBibTeX::tfPerson: return KIcon("user-identity");
    case KBibTeX::tfKeyword: return KIcon("edit-find");
    case KBibTeX::tfSource: return KIcon("code-context");
    default: return KIcon();
    };
}


void FieldLineEdit::loadValue(const Value& value)
{
    QString text = "";
    m_incompleteRepresentation = false;

    if (value.size() > 0) {
        if (m_typeFlag == KBibTeX::tfSource) {
            FileExporterBibTeX *exporter = new FileExporterBibTeX();
            text = exporter->valueToBibTeX(value);
            delete exporter;
        } else {
            m_incompleteRepresentation = value.size() > 1;
            const ValueItem *first = value.first();
            const PlainText *plainText = dynamic_cast<const PlainText*>(first);
            if (plainText != NULL) {
                text = plainText->text();
            } else {
                const Person *person = dynamic_cast<const Person*>(first);
                if (person != NULL) {
                    text = person->lastName();
                    QString temp = person->firstName();
                    if (!temp.isEmpty()) text.prepend(temp + " ");
                    temp = person->suffix();
                    if (!temp.isEmpty()) text.append(", " + temp);
                    temp = person->prefix();
                    if (!temp.isEmpty()) text.prepend(temp + " ");
                } else {
                    const MacroKey *macroKey = dynamic_cast<const MacroKey*>(first);
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

    if (m_typeFlags&KBibTeX::tfText) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(KBibTeX::tfText), i18n("Plain Text"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, KBibTeX::tfText);
    }
    if (m_typeFlags&KBibTeX::tfReference) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(KBibTeX::tfReference), i18n("Reference"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, KBibTeX::tfReference);
    }
    if (m_typeFlags&KBibTeX::tfPerson) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(KBibTeX::tfPerson), i18n("Person"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, KBibTeX::tfPerson);
    }
    if (m_typeFlags&KBibTeX::tfSource) {
        QAction *action = m_menuTypes->addAction(iconForTypeFlag(KBibTeX::tfSource), i18n("Source Code"), m_menuTypesSignalMapper, SLOT(map()));
        m_menuTypesSignalMapper->setMapping(action, KBibTeX::tfSource);
    }
}

void FieldLineEdit::updateGUI()
{
    setFont(KGlobalSettings::generalFont());
    setIcon(iconForTypeFlag(m_typeFlag));
    switch (m_typeFlag) {
    case KBibTeX::tfText: setButtonToolTip(i18n("Plain Text")); break;
    case KBibTeX::tfReference: setButtonToolTip(i18n("Reference")); break;
    case KBibTeX::tfPerson: setButtonToolTip(i18n("Person")); break;
    case KBibTeX::tfSource:
        setButtonToolTip(i18n("Source Code"));
        setFont(KGlobalSettings::fixedFont());
        break;
    default: setButtonToolTip(""); break;
    };
}

void FieldLineEdit::slotTypeChanged(int newTypeFlag)
{
    Value value = m_originalValue;
    if (!m_incompleteRepresentation)
        applyTo(value);

    m_typeFlag = (KBibTeX::TypeFlag)newTypeFlag;
    updateGUI();

    loadValue(value);
}
