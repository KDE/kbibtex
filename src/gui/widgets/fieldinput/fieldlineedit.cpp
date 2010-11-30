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
#include <QFileInfo>
#include <QDesktopServices>

#include <KDebug>
#include <KMessageBox>
#include <KGlobalSettings>
#include <KLocale>
#include <KUrl>
#include <KPushButton>

#include <file.h>
#include <entry.h>
#include <value.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <bibtexfields.h>
#include <encoderlatex.h>
#include "fieldlineedit.h"

class FieldLineEdit::FieldLineEditPrivate
{
private:
    FieldLineEdit *parent;
    Value currentValue;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;
    QSignalMapper *menuTypesSignalMapper;
    KPushButton *buttonOpenUrl;

public:
    QMenu *menuTypes;
    KBibTeX::TypeFlag typeFlag;
    KUrl urlToOpen;

    FieldLineEditPrivate(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldLineEdit *p)
            : parent(p), preferredTypeFlag(ptf), typeFlags(tf) {
        menuTypes = new QMenu(i18n("Types"), parent);
        menuTypesSignalMapper = new QSignalMapper(parent);
        setupMenu();
        connect(menuTypesSignalMapper, SIGNAL(mapped(int)), parent, SLOT(slotTypeChanged(int)));

        buttonOpenUrl = new KPushButton(KIcon("document-open-remote"), "", parent);
        buttonOpenUrl->setVisible(false);
        parent->appendWidget(buttonOpenUrl);
        connect(buttonOpenUrl, SIGNAL(clicked()), parent, SLOT(slotOpenUrl()));

        connect(p, SIGNAL(textChanged(QString)), p, SLOT(slotTextChanged(QString)));

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
                            } else {
                                const VerbatimText *verbatimText = dynamic_cast<const VerbatimText*>(first);
                                if (typeFlag == KBibTeX::tfVerbatim && verbatimText != NULL) {
                                    text = verbatimText->text();
                                    result = true;
                                } else
                                    kWarning() << "Could not reset: " << typeFlag << " " << " " << typeid(*first).name();
                            }
                        }
                    }
                }
            }
        }

        updateURL(text);

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
        } else if (typeFlag == KBibTeX::tfVerbatim) {
            value.append(new VerbatimText(text));
            return true;
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
        else if (typeFlag == KBibTeX::tfVerbatim && typeid(VerbatimText) == typeid(value.first()))
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
        if (typeFlags.testFlag(KBibTeX::tfVerbatim)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::tfVerbatim), i18n("Verbatim Text"), menuTypesSignalMapper, SLOT(map()));
            menuTypesSignalMapper->setMapping(action, KBibTeX::tfVerbatim);
        }
    }

    KIcon iconForTypeFlag(KBibTeX::TypeFlag typeFlag) {
        switch (typeFlag) {
        case KBibTeX::tfPlainText: return KIcon("draw-text");
        case KBibTeX::tfReference: return KIcon("emblem-symbolic-link");
        case KBibTeX::tfPerson: return KIcon("user-identity");
        case KBibTeX::tfKeyword: return KIcon("edit-find");
        case KBibTeX::tfSource: return KIcon("code-context");
        case KBibTeX::tfVerbatim: return KIcon("preferences-desktop-keyboard");
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
        case KBibTeX::tfVerbatim: parent->setButtonToolTip(i18n("Verbatim Text")); break;
        default: parent->setButtonToolTip(""); break;
        };
    }

    void openUrl() {
        if (urlToOpen.isValid())
            QDesktopServices::openUrl(urlToOpen); // TODO KDE way?
    }

    bool convertValueType(Value &value, KBibTeX::TypeFlag destType) {
        if (value.isEmpty()) return true; /// simple case
        if (destType == KBibTeX::tfSource) return true; /// simple case

        bool result = true;
        EncoderLaTeX *enc = EncoderLaTeX::currentEncoderLaTeX();
        QString rawText = QString::null;
        const ValueItem *first = value.first();

        const PlainText *plainText = dynamic_cast<const PlainText*>(first);
        if (plainText != NULL)
            rawText = enc->encode(plainText->text());
        else {
            const VerbatimText *verbatimText = dynamic_cast<const VerbatimText*>(first);
            if (verbatimText != NULL)
                rawText = verbatimText->text();
            else {
                const MacroKey *macroKey = dynamic_cast<const MacroKey*>(first);
                if (macroKey != NULL)
                    rawText = macroKey->text();
                else {
                    const Person *person = dynamic_cast<const Person*>(first);
                    if (person != NULL)
                        rawText = enc->encode(QString("%1 %2").arg(person->firstName()).arg(person->lastName())); // FIXME proper name conversion
                    else {
                        const Keyword *keyword = dynamic_cast<const Keyword*>(first);
                        if (keyword != NULL)
                            rawText = enc->encode(keyword->text());
                        else {
                            // TODO case missed?
                            result = false;
                        }
                    }
                }
            }
        }

        switch (destType) {
        case KBibTeX::tfPlainText:
            value.clear();
            value.append(new PlainText(enc->decode(rawText)));
            break;
        case KBibTeX::tfVerbatim:
            value.clear();
            value.append(new VerbatimText(rawText));
            break;
        case KBibTeX::tfPerson:
            value.clear();
            value.append(FileImporterBibTeX::splitName(enc->decode(rawText)));
            break;
        case KBibTeX::tfReference: {
            MacroKey *macroKey = new MacroKey(rawText);
            if (macroKey->isValid()) {
                value.clear();
                value.append(macroKey);
            } else {
                delete macroKey;
                result = false;
            }
        }
        break;
        case KBibTeX::tfKeyword:
            value.append(new Keyword(enc->decode(rawText)));
            break;
        default: {
            // TODO
            result = false;
        }
        }

        return result;
    }

    void updateURL(const QString &text) {
        urlToOpen = KUrl();

        /// extract URL, local file, or DOI from current field
        if (KBibTeX::urlRegExp.indexIn(text) > -1)
            urlToOpen = KUrl(KBibTeX::urlRegExp.cap(0));
        else if (KBibTeX::doiRegExp.indexIn(text) > -1)
            urlToOpen = KUrl(KBibTeX::doiUrlPrefix + KBibTeX::doiRegExp.cap(0).replace("\\", ""));
        else if (KBibTeX::fileRegExp.indexIn(text) > -1)
            urlToOpen = KUrl(KBibTeX::fileRegExp.cap(0));

        /// non-existing local files are to be ignored
        if (urlToOpen.isValid() && urlToOpen.isLocalFile() && !QFileInfo(urlToOpen.path()).exists())
            urlToOpen = KUrl();

        /// set special "open URL" button visible if URL (or file or DOI) found
        buttonOpenUrl->setVisible(urlToOpen.isValid());
        buttonOpenUrl->setToolTip(i18n("Open \"%1\"", urlToOpen.pathOrUrl()));
    }

    void textChanged(const QString &text) {
        updateURL(text);
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

void FieldLineEdit::slotTypeChanged(int newTypeFlagInt)
{
    KBibTeX::TypeFlag newTypeFlag = (KBibTeX::TypeFlag)newTypeFlagInt;
    kDebug() << "new type is " << BibTeXFields::typeFlagToString(newTypeFlag);

    Value value;
    d->apply(value);

    if (d->convertValueType(value, newTypeFlag)) {
        d->typeFlag = newTypeFlag;
        d->reset(value);
    } else
        KMessageBox::error(this, i18n("The current text cannot be used as value of type \"%1\".\n\nSwitching back to type \"%2\".", BibTeXFields::typeFlagToString(newTypeFlag), BibTeXFields::typeFlagToString(d->typeFlag)));
}

void FieldLineEdit::slotOpenUrl()
{
    d->openUrl();
}

void FieldLineEdit::slotTextChanged(const QString &text)
{
    d->textChanged(text);
}
