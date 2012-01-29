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
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>

#include <KMimeType>
#include <KRun>
#include <KDebug>
#include <KMessageBox>
#include <KGlobalSettings>
#include <KLocale>
#include <KUrl>
#include <KPushButton>
#include <KSharedConfig>
#include <KConfigGroup>

#include <fileinfo.h>
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

    KSharedConfigPtr config;
    const QString configGroupNameGeneral;
    QString personNameFormatting;

public:
    QMenu *menuTypes;
    KBibTeX::TypeFlag typeFlag;
    KUrl urlToOpen;
    const File *file;
    QString fieldKey;

    FieldLineEditPrivate(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldLineEdit *p)
            : parent(p), preferredTypeFlag(ptf), typeFlags(tf), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupNameGeneral(QLatin1String("General")), file(NULL) {
        menuTypes = new QMenu(i18n("Types"), parent);
        menuTypesSignalMapper = new QSignalMapper(parent);
        setupMenu();
        connect(menuTypesSignalMapper, SIGNAL(mapped(int)), parent, SLOT(slotTypeChanged(int)));

        buttonOpenUrl = new KPushButton(KIcon("document-open-remote"), "", parent);
        buttonOpenUrl->setVisible(false);
        buttonOpenUrl->setProperty("isConst", true);
        parent->appendWidget(buttonOpenUrl);
        connect(buttonOpenUrl, SIGNAL(clicked()), parent, SLOT(slotOpenUrl()));

        connect(p, SIGNAL(textChanged(QString)), p, SLOT(slotTextChanged(QString)));

        Value value;
        typeFlag = determineTypeFlag(value, preferredTypeFlag, typeFlags);
        updateGUI(typeFlag);

        KConfigGroup configGroup(config, configGroupNameGeneral);
        personNameFormatting = configGroup.readEntry(Person::keyPersonNameFormatting, Person::defaultPersonNameFormatting);
    }

    ~FieldLineEditPrivate() {
        delete menuTypes;
        delete menuTypesSignalMapper;
        delete buttonOpenUrl;
    }

    bool reset(const Value& value) {
        bool result = false;
        QString text = "";
        typeFlag = determineTypeFlag(value, typeFlag, typeFlags);
        updateGUI(typeFlag);

        if (!value.isEmpty()) {
            if (typeFlag == KBibTeX::tfSource) {
                /// simple case: field's value is to be shown as BibTeX code, including surrounding curly braces
                FileExporterBibTeX exporter;
                text = exporter.valueToBibTeX(value);
                result = true;
            } else {
                /// except for the source view type flag, type flag views do not support composed values,
                /// therefore only the first value will be shown
                const QSharedPointer<ValueItem> first = value.first();

                const QSharedPointer<PlainText> plainText = first.dynamicCast<PlainText>();
                if (typeFlag == KBibTeX::tfPlainText && !plainText.isNull()) {
                    text = plainText->text();
                    result = true;
                } else {
                    const QSharedPointer<Person> person = first.dynamicCast<Person>();
                    if (typeFlag == KBibTeX::tfPerson && !person.isNull()) {
                        text = Person::transcribePersonName(person.data(), personNameFormatting);
                        result = true;
                    } else {
                        const QSharedPointer<MacroKey> macroKey = first.dynamicCast<MacroKey>();
                        if (typeFlag == KBibTeX::tfReference && !macroKey.isNull()) {
                            text = macroKey->text();
                            result = true;
                        } else {
                            const QSharedPointer<Keyword> keyword = first.dynamicCast<Keyword>();
                            if (typeFlag == KBibTeX::tfKeyword && !keyword.isNull()) {
                                text = keyword->text();
                                result = true;
                            } else {
                                const QSharedPointer<VerbatimText> verbatimText = first.dynamicCast<VerbatimText>();
                                if (typeFlag == KBibTeX::tfVerbatim && !verbatimText.isNull()) {
                                    text = verbatimText->text();
                                    result = true;
                                } else
                                    kWarning() << "Could not reset: " << typeFlag << "(" << (typeFlag == KBibTeX::tfSource ? "Source" : (typeFlag == KBibTeX::tfReference ? "Reference" : (typeFlag == KBibTeX::tfPerson ? "Person" : (typeFlag == KBibTeX::tfPlainText ? "PlainText" : (typeFlag == KBibTeX::tfKeyword ? "Keyword" : (typeFlag == KBibTeX::tfVerbatim ? "Verbatim" : "???")))))) << ") " << (typeFlags.testFlag(KBibTeX::tfPerson) ? "Person" : "") << (typeFlags.testFlag(KBibTeX::tfPlainText) ? "PlainText" : "") << (typeFlags.testFlag(KBibTeX::tfReference) ? "Reference" : "") << (typeFlags.testFlag(KBibTeX::tfVerbatim) ? "Verbatim" : "") << " " << typeid(*first).name() << " : " << PlainTextValue::text(value);
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
        const QString text = parent->text();

        EncoderLaTeX *encoder = EncoderLaTeX::instance();
        const QString encodedText = encoder->decode(text);
        if (encodedText != text)
            parent->setText(encodedText);

        if (encodedText.isEmpty())
            return true;
        else if (typeFlag == KBibTeX::tfPlainText) {
            value.append(QSharedPointer<PlainText>(new PlainText(encodedText)));
            return true;
        } else if (typeFlag == KBibTeX::tfReference && !encodedText.contains(QRegExp("[^-_:/a-zA-Z0-9]"))) {
            value.append(QSharedPointer<MacroKey>(new MacroKey(encodedText)));
            return true;
        } else if (typeFlag == KBibTeX::tfPerson) {
            value.append(FileImporterBibTeX::personFromString(encodedText));
            return true;
        } else if (typeFlag == KBibTeX::tfKeyword) {
            QList<Keyword*> keywords = FileImporterBibTeX::splitKeywords(encodedText);
            for (QList<Keyword*>::Iterator it = keywords.begin(); it != keywords.end(); ++it)
                value.append(QSharedPointer<Keyword>(*it));
            return true;
        } else if (typeFlag == KBibTeX::tfSource) {
            QString key = typeFlags.testFlag(KBibTeX::tfPerson) ? "author" : "title";
            FileImporterBibTeX importer;
            QString fakeBibTeXFile = QString("@article{dummy, %1=%2}").arg(key).arg(encodedText);

            File *file = importer.fromString(fakeBibTeXFile);
            QSharedPointer<Entry> entry;
            if (file != NULL) {
                if (!file->isEmpty() && !(entry = (file->first().dynamicCast<Entry>())).isNull())
                    value = entry->value(key);
                delete file;
            }
            if (entry.isNull())
                kWarning() << "Parsing " << fakeBibTeXFile << " did not result in valid entry";
            return !value.isEmpty();
        } else if (typeFlag == KBibTeX::tfVerbatim) {
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(encodedText)));
            return true;
        }

        return false;
    }

    KBibTeX::TypeFlag determineTypeFlag(const Value &value, KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags availableTypeFlags) {
        KBibTeX::TypeFlag result = KBibTeX::tfSource;
        if (availableTypeFlags.testFlag(preferredTypeFlag) && typeFlagSupported(value, preferredTypeFlag))
            result = preferredTypeFlag;
        else if (value.count() == 1) {
            int p = 1;
            for (int i = 1; i < 8; ++i, p <<= 1) {
                KBibTeX::TypeFlag flag = (KBibTeX::TypeFlag)p;
                if (availableTypeFlags.testFlag(flag) && typeFlagSupported(value, flag)) {
                    result = flag; break;
                }
            }
        }
        return result;
    }

    bool typeFlagSupported(const Value &value, KBibTeX::TypeFlag typeFlag) {
        if (value.isEmpty() || typeFlag == KBibTeX::tfSource)
            return true;
        else if (value.count() > 1)
            return typeFlag == KBibTeX::tfSource;
        else if (typeFlag == KBibTeX::tfKeyword && typeid(Keyword) == typeid(*value.first()))
            return true;
        else if (typeFlag == KBibTeX::tfPerson && typeid(Person) == typeid(*value.first()))
            return true;
        else if (typeFlag == KBibTeX::tfPlainText && typeid(PlainText) == typeid(*value.first()))
            return true;
        else if (typeFlag == KBibTeX::tfReference && typeid(MacroKey) == typeid(*value.first()))
            return true;
        else if (typeFlag == KBibTeX::tfVerbatim && typeid(VerbatimText) == typeid(*value.first()))
            return true;
        else
            return false;
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
        if (urlToOpen.isValid()) {
            /// Guess mime type for url to open
            KMimeType::Ptr mimeType = KMimeType::findByPath(urlToOpen.path());
            QString mimeTypeName = mimeType->name();
            if (mimeTypeName == QLatin1String("application/octet-stream"))
                mimeTypeName = QLatin1String("text/html");
            /// Ask KDE subsystem to open url in viewer matching mime type
            KRun::runUrl(urlToOpen, mimeTypeName, parent, false, false);
        }
    }

    bool convertValueType(Value &value, KBibTeX::TypeFlag destType) {
        if (value.isEmpty()) return true; /// simple case
        if (destType == KBibTeX::tfSource) return true; /// simple case

        bool result = true;
        EncoderLaTeX *enc = EncoderLaTeX::instance();
        QString rawText = QString::null;
        const QSharedPointer<ValueItem> first = value.first();

        const QSharedPointer<PlainText> plainText = first.dynamicCast<PlainText>();
        if (!plainText.isNull())
            rawText = enc->encode(plainText->text());
        else {
            const QSharedPointer<VerbatimText> verbatimText = first.dynamicCast<VerbatimText>();
            if (!verbatimText.isNull())
                rawText = verbatimText->text();
            else {
                const QSharedPointer<MacroKey> macroKey = first.dynamicCast<MacroKey>();
                if (!macroKey.isNull())
                    rawText = macroKey->text();
                else {
                    const QSharedPointer<Person> person = first.dynamicCast<Person>();
                    if (!person.isNull())
                        rawText = enc->encode(QString("%1 %2").arg(person->firstName()).arg(person->lastName())); // FIXME proper name conversion
                    else {
                        const QSharedPointer<Keyword> keyword = first.dynamicCast<Keyword>();
                        if (!keyword.isNull())
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
            value.append(QSharedPointer<PlainText>(new PlainText(enc->decode(rawText))));
            break;
        case KBibTeX::tfVerbatim:
            value.clear();
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(rawText)));
            break;
        case KBibTeX::tfPerson:
            value.clear();
            value.append(QSharedPointer<Person>(FileImporterBibTeX::splitName(enc->decode(rawText))));
            break;
        case KBibTeX::tfReference: {
            MacroKey *macroKey = new MacroKey(rawText);
            if (macroKey->isValid()) {
                value.clear();
                value.append(QSharedPointer<MacroKey>(macroKey));
            } else {
                delete macroKey;
                result = false;
            }
        }
        break;
        case KBibTeX::tfKeyword:
            value.clear();
            value.append(QSharedPointer<Keyword>(new Keyword(enc->decode(rawText))));
            break;
        default: {
            // TODO
            result = false;
        }
        }

        return result;
    }

    void updateURL(const QString &text) {
        QList<KUrl> urls;
        FileInfo::urlsInText(text, FileInfo::TestExistanceYes, file != NULL && file->property(File::Url).toUrl().isValid() ? KUrl(file->property(File::Url).toUrl()).directory() : QString::null, urls);
        if (!urls.isEmpty() && urls.first().isValid())
            urlToOpen = urls.first();
        else
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
    setChildAcceptDrops(false);
    setAcceptDrops(true);
}

FieldLineEdit::~FieldLineEdit()
{
    delete d;
}

bool FieldLineEdit::apply(Value& value) const
{
    return d->apply(value);
}

bool FieldLineEdit::reset(const Value& value)
{
    return d->reset(value);
}

void FieldLineEdit::setReadOnly(bool isReadOnly)
{
    MenuLineEdit::setReadOnly(isReadOnly);
}

void FieldLineEdit::slotTypeChanged(int newTypeFlagInt)
{
    KBibTeX::TypeFlag newTypeFlag = (KBibTeX::TypeFlag)newTypeFlagInt;

    Value value;
    d->apply(value);

    if (d->convertValueType(value, newTypeFlag)) {
        d->typeFlag = newTypeFlag;
        d->reset(value);
    } else
        KMessageBox::error(this, i18n("The current text cannot be used as value of type \"%1\".\n\nSwitching back to type \"%2\".", BibTeXFields::typeFlagToString(newTypeFlag), BibTeXFields::typeFlagToString(d->typeFlag)));
}

void FieldLineEdit::setFile(const File *file)
{
    d->file = file;
}

void FieldLineEdit::setElement(const Element *element)
{
    Q_UNUSED(element)
}

void FieldLineEdit::setFieldKey(const QString &fieldKey)
{
    d->fieldKey = fieldKey;
}

void FieldLineEdit::slotOpenUrl()
{
    d->openUrl();
}

void FieldLineEdit::slotTextChanged(const QString &text)
{
    d->textChanged(text);
}

void FieldLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain") || event->mimeData()->hasFormat("text/x-bibtex"))
        event->acceptProposedAction();
}

void FieldLineEdit::dropEvent(QDropEvent *event)
{
    const QString clipboardText = event->mimeData()->text();
    if (clipboardText.isEmpty()) return;

    const File *file = NULL;
    if (!d->fieldKey.isEmpty() && clipboardText.startsWith("@")) {
        FileImporterBibTeX importer;
        file = importer.fromString(clipboardText);
        const QSharedPointer<Entry> entry = (file != NULL && file->count() == 1) ? file->first().dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull() && d->fieldKey == Entry::ftCrossRef) {
            /// handle drop on crossref line differently (use dropped entry's id)
            Value v;
            v.append(QSharedPointer<VerbatimText>(new VerbatimText(entry->id())));
            reset(v);
            emit textChanged(entry->id());
            return;
        } else if (!entry.isNull() && entry->contains(d->fieldKey)) {
            /// case for "normal" fields like for journal, pages, ...
            reset(entry->value(d->fieldKey));
            emit textChanged(text());
            return;
        }
    }

    if (file == NULL || file->count() == 0) {
        /// fall-back case: just copy whole text into edit widget
        setText(clipboardText);
        emit textChanged(clipboardText);
    }
}
