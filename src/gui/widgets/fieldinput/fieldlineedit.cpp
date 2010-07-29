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
#include <value.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <bibtexfields.h>
#include "fieldlineedit.h"

class FieldLineEdit::FieldLineEditPrivate
{
private:
    FieldLineEdit *parent;
    Value currentValue;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;
    QSignalMapper *menuTypesSignalMapper;

public:
    QMenu *menuTypes;
    KBibTeX::TypeFlag typeFlag;

    FieldLineEditPrivate(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldLineEdit *p)
            : parent(p), preferredTypeFlag(ptf), typeFlags(tf) {
        menuTypes = new QMenu(i18n("Types"), parent);
        menuTypesSignalMapper = new QSignalMapper(parent);
        setupMenu();
        connect(menuTypesSignalMapper, SIGNAL(mapped(int)), parent, SLOT(slotTypeChanged(int)));

        Value value;
        typeFlag = determineTypeFlag(value, preferredTypeFlag, typeFlags);
        updateGUI(typeFlag);
    }

    void reset(const Value& value) {
        QString text = "";
        typeFlag = determineTypeFlag(value, typeFlag, typeFlags);
        updateGUI(typeFlag);

        if (!value.isEmpty()) {
            if (typeFlag == KBibTeX::tfSource) {
                FileExporterBibTeX exporter;
                text = exporter.valueToBibTeX(value);
            } else {
                const ValueItem *first = value.first();
                const PlainText *plainText = dynamic_cast<const PlainText*>(first);
                if (typeFlag == KBibTeX::tfPlainText && plainText != NULL) {
                    text = plainText->text();
                } else {
                    const Person *person = dynamic_cast<const Person*>(first);
                    if (typeFlag == KBibTeX::tfPerson && person != NULL) {
                        text = person->lastName();
                        QString temp = person->firstName();
                        if (!temp.isEmpty()) text.prepend(temp + " ");
                        temp = person->suffix();
                        if (!temp.isEmpty()) text.append(", " + temp);
                        temp = person->prefix();
                        if (!temp.isEmpty()) text.prepend(temp + " ");
                    } else {
                        const MacroKey *macroKey = dynamic_cast<const MacroKey*>(first);
                        if (typeFlag == KBibTeX::tfReference && macroKey != NULL) {
                            text = macroKey->text();
                        } else {
                            const Keyword *keyword = dynamic_cast<const Keyword*>(first);
                            if (typeFlag == KBibTeX::tfKeyword && keyword != NULL) {
                                text = keyword->text();
                            }
                        }
                    }
                }
            }
        }

        parent->setText(text);
    }

    bool apply(Value& value) const {
        value.clear();
        QString text = parent->text();

        if (text.isEmpty())
            return true;
        else if (typeFlag == KBibTeX::tfPlainText) {
            value.append(new PlainText(text));
            return true;
        } else if (typeFlag == KBibTeX::tfReference && !text.contains(QRegExp("[^-_:/a-zA-Z0-9]"))) {
            value.append(new MacroKey(text));
            return true;
        } else if (typeFlag == KBibTeX::tfPerson) {
            value.append(FileImporterBibTeX::splitName(text));
            return true;
        } else if (typeFlag == KBibTeX::tfKeyword) {
            QList<Keyword*> keywords = FileImporterBibTeX::splitKeywords(text);
            for (QList<Keyword*>::Iterator it = keywords.begin(); it != keywords.end(); ++it)
                value.append(*it);
            return true;
        } else if (typeFlag == KBibTeX::tfSource) {
            QString key = typeFlags.testFlag(KBibTeX::tfPerson) ? "author" : "title";
            FileImporterBibTeX importer;
            QString fakeBibTeXFile = QString("@article{dummy, %1=%2}").arg(key).arg(text);
            kDebug() << "fakeBibTeXFile=" << fakeBibTeXFile << endl;

            File *file = importer.fromString(fakeBibTeXFile);
            if (file != NULL) {
                Entry *entry = dynamic_cast< Entry*>(file->first());
                if (entry != NULL) {
                    value = entry->value(key);
                    kDebug() << "value->count()=" << value.count() << "  " << entry->value(key).count();
                } else
                    kError() << "Cannot create value";
                delete file;
            }
            return !value.isEmpty();
        }

        return false;
    }

    KBibTeX::TypeFlag determineTypeFlag(const Value &value, KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags availableTypeFlags) {
        if (value.count() > 1) {
            return KBibTeX::tfSource;
        } else {
            KBibTeX::TypeFlag result = preferredTypeFlag;
            int p = 1;
            for (int i = 1; i < 8; ++i, p <<= 1) {
                KBibTeX::TypeFlag flag = (KBibTeX::TypeFlag)p;
                if (availableTypeFlags.testFlag(flag) && typeFlagSupported(value, flag))
                    result = flag;
            }
            if (availableTypeFlags.testFlag(preferredTypeFlag) && typeFlagSupported(value, preferredTypeFlag))
                result = preferredTypeFlag;

            return result;
        }
    }

    bool typeFlagSupported(const Value &value, KBibTeX::TypeFlag typeFlag) {
        if (value.isEmpty() || typeFlag == KBibTeX::tfSource)
            return true;
        else if (value.count() > 1)
            return typeFlag == KBibTeX::tfSource;
        else if (typeFlag == KBibTeX::tfKeyword && dynamic_cast<Keyword*>(value.first()) != NULL)
            return true;
        else if (typeFlag == KBibTeX::tfPerson && dynamic_cast<Person*>(value.first()) != NULL)
            return true;
        else if (typeFlag == KBibTeX::tfPlainText && dynamic_cast<PlainText*>(value.first()) != NULL)
            return true;
        else if (typeFlag == KBibTeX::tfReference && dynamic_cast<MacroKey*>(value.first()) != NULL)
            return true;
        else return false;
    }


    void setupMenu() {
        menuTypes->clear();

        if (typeFlags.testFlag(KBibTeX::tfPlainText)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::tfPlainText), i18n("Plain Text"), menuTypesSignalMapper, SLOT(map()));
            menuTypesSignalMapper->setMapping(action, KBibTeX::tfPlainText);
        }
        if (typeFlags.testFlag(KBibTeX::tfReference)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::tfReference), i18n("Reference"), menuTypesSignalMapper, SLOT(map()));
            menuTypesSignalMapper->setMapping(action, KBibTeX::tfReference);
        }
        if (typeFlags.testFlag(KBibTeX::tfPerson)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::tfPerson), i18n("Person"), menuTypesSignalMapper, SLOT(map()));
            menuTypesSignalMapper->setMapping(action, KBibTeX::tfPerson);
        }
        if (typeFlags.testFlag(KBibTeX::tfKeyword)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::tfKeyword), i18n("Keyword"), menuTypesSignalMapper, SLOT(map()));
            menuTypesSignalMapper->setMapping(action, KBibTeX::tfKeyword);
        }
        if (typeFlags.testFlag(KBibTeX::tfSource)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::tfSource), i18n("Source Code"), menuTypesSignalMapper, SLOT(map()));
            menuTypesSignalMapper->setMapping(action, KBibTeX::tfSource);
        }
    }

    KIcon iconForTypeFlag(KBibTeX::TypeFlag typeFlag) {
        switch (typeFlag) {
        case KBibTeX::tfPlainText: return KIcon("draw-text");
        case KBibTeX::tfReference: return KIcon("emblem-symbolic-link");
        case KBibTeX::tfPerson: return KIcon("user-identity");
        case KBibTeX::tfKeyword: return KIcon("edit-find");
        case KBibTeX::tfSource: return KIcon("code-context");
        default: return KIcon();
        };
    }

    void updateGUI(KBibTeX::TypeFlag typeFlag) {
        parent->setFont(KGlobalSettings::generalFont());
        parent->setIcon(iconForTypeFlag(typeFlag));
        switch (typeFlag) {
        case KBibTeX::tfPlainText: parent->setButtonToolTip(i18n("Plain Text")); break;
        case KBibTeX::tfReference: parent->setButtonToolTip(i18n("Reference")); break;
        case KBibTeX::tfPerson: parent->setButtonToolTip(i18n("Person")); break;
        case KBibTeX::tfKeyword: parent->setButtonToolTip(i18n("Keyword")); break;
        case KBibTeX::tfSource:
            parent->setButtonToolTip(i18n("Source Code"));
            parent->setFont(KGlobalSettings::fixedFont());
            break;
        default: parent->setButtonToolTip(""); break;
        };
    }

};

FieldLineEdit::FieldLineEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, bool isMultiLine, QWidget *parent)
        : MenuLineEdit(isMultiLine, parent), d(new FieldLineEdit::FieldLineEditPrivate(preferredTypeFlag, typeFlags, this))
{
    setObjectName(QLatin1String("FieldLineEdit"));
    setMenu(d->menuTypes);
}

void FieldLineEdit::apply(Value& value) const
{
    d->apply(value);
}

void FieldLineEdit::reset(const Value& value)
{
    d->reset(value);
}

void FieldLineEdit::slotTypeChanged(int newTypeFlag)
{
    KBibTeX::TypeFlag originalTypeFlag = d->typeFlag;
    Value originalValue;
    d->apply(originalValue);

    d->typeFlag = (KBibTeX::TypeFlag)newTypeFlag;
    kDebug() << "new type is " << BibTeXFields::typeFlagToString(d->typeFlag);

    Value testValue;
    if (d->apply(testValue))
        d->reset(testValue);
    else {
        KMessageBox::error(this, i18n("The current text cannot be used as value of type \"%1\".\n\nSwitching back to type \"%2\".", BibTeXFields::typeFlagToString(d->typeFlag), BibTeXFields::typeFlagToString(originalTypeFlag)));
        d->typeFlag = originalTypeFlag;
        d->reset(originalValue);
    }
}
