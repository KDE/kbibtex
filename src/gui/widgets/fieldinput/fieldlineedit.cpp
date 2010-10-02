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

    bool reset(const Value& value) {
        bool result = false;
        QString text = "";
        typeFlag = determineTypeFlag(value, typeFlag, typeFlags);
        updateGUI(typeFlag);

        if (!value.isEmpty()) {
            if (typeFlag == KBibTeX::tfSource) {
                FileExporterBibTeX exporter;
                text = exporter.valueToBibTeX(value);
                result = true;
            } else {
                const ValueItem *first = value.first();
                const PlainText *plainText = dynamic_cast<const PlainText*>(first);
                if (typeFlag == KBibTeX::tfPlainText && plainText != NULL) {
                    text = plainText->text();
                    result = true;
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
                        result = true;
                    } else {
                        const MacroKey *macroKey = dynamic_cast<const MacroKey*>(first);
                        if (typeFlag == KBibTeX::tfReference && macroKey != NULL) {
                            text = macroKey->text();
                            result = true;
                        } else {
                            const Keyword *keyword = dynamic_cast<const Keyword*>(first);
                            if (typeFlag == KBibTeX::tfKeyword && keyword != NULL) {
                                text = keyword->text();
                                result = true;
                            }
                        }
                    }
                }
            }
        }

        parent->setText(text);
        return result;
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
            if (file != NULL && !file->isEmpty()) {
                Entry *entry = dynamic_cast< Entry*>(file->first());
                if (entry != NULL) {
                    value = entry->value(key);
                    kDebug() << "value->count()=" << value.count() << "  " << entry->value(key).count();
                } else
                    kError() << "Cannot create value";
                delete file;
            } else
                kWarning() << "Parsing " << fakeBibTeXFile << " did not result in valid file";
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
        else if (typeFlag == KBibTeX::tfKeyword && typeid(Keyword) == typeid(value.first()))
            return true;
        else if (typeFlag == KBibTeX::tfPerson && typeid(Person) == typeid(value.first()))
            return true;
        else if (typeFlag == KBibTeX::tfPlainText && typeid(PlainText) == typeid(value.first()))
            return true;
        else if (typeFlag == KBibTeX::tfReference && typeid(MacroKey) == typeid(value.first()))
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
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    setObjectName(QLatin1String("FieldLineEdit"));
    setMenu(d->menuTypes);
}

bool FieldLineEdit::apply(Value& value) const
{
    return d->apply(value);
}

bool FieldLineEdit::reset(const Value& value)
{
    return d->reset(value);
}

void FieldLineEdit::slotTypeChanged(int newTypeFlag)
{
    KBibTeX::TypeFlag originalTypeFlag = d->typeFlag;
    Value originalValue;
    d->apply(originalValue);

    d->typeFlag = (KBibTeX::TypeFlag)newTypeFlag;
    kDebug() << "new type is " << BibTeXFields::typeFlagToString(d->typeFlag);

    if (!d->reset(originalValue)) {
        KMessageBox::error(this, i18n("The current text cannot be used as value of type \"%1\".\n\nSwitching back to type \"%2\".", BibTeXFields::typeFlagToString(d->typeFlag), BibTeXFields::typeFlagToString(originalTypeFlag)));
        d->typeFlag = originalTypeFlag;
        d->reset(originalValue);
    }
}
