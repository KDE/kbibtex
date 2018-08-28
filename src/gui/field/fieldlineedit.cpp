/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fieldlineedit.h"

#include <typeinfo>

#include <QMenu>
#include <QSignalMapper>
#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPushButton>
#include <QFontDatabase>
#include <QUrl>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMimeData>
#include <QRegularExpression>

#include <KRun>
#include <KMessageBox>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#include <kio_version.h>

#include "fileinfo.h"
#include "file.h"
#include "entry.h"
#include "value.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "bibtexfields.h"
#include "encoderlatex.h"
#include "preferences.h"
#include "logging_gui.h"

class FieldLineEdit::FieldLineEditPrivate
{
private:
    FieldLineEdit *parent;
    Value currentValue;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;
    QSignalMapper *menuTypesSignalMapper;
    QPushButton *buttonOpenUrl;

    KSharedConfigPtr config;
    const QString configGroupNameGeneral;
    QString personNameFormatting;

public:
    QMenu *menuTypes;
    KBibTeX::TypeFlag typeFlag;
    QUrl urlToOpen;
    const File *file;
    QString fieldKey;

    FieldLineEditPrivate(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldLineEdit *p)
            : parent(p), preferredTypeFlag(ptf), typeFlags(tf), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupNameGeneral(QStringLiteral("General")), file(nullptr) {
        menuTypes = new QMenu(parent);
        menuTypesSignalMapper = new QSignalMapper(parent);
        setupMenu();
        connect(menuTypesSignalMapper, static_cast<void(QSignalMapper::*)(int)>(&QSignalMapper::mapped), parent, &FieldLineEdit::slotTypeChanged);

        buttonOpenUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open-remote")), QStringLiteral(""), parent);
        buttonOpenUrl->setVisible(false);
        buttonOpenUrl->setProperty("isConst", true);
        parent->appendWidget(buttonOpenUrl);
        connect(buttonOpenUrl, &QPushButton::clicked, parent, &FieldLineEdit::slotOpenUrl);

        connect(p, &FieldLineEdit::textChanged, p, &FieldLineEdit::slotTextChanged);

        Value value;
        typeFlag = determineTypeFlag(value, preferredTypeFlag, typeFlags);
        updateGUI(typeFlag);

        KConfigGroup configGroup(config, configGroupNameGeneral);
        personNameFormatting = configGroup.readEntry(Preferences::keyPersonNameFormatting, Preferences::defaultPersonNameFormatting);
    }

    bool reset(const Value &value) {
        bool result = false;
        QString text;
        typeFlag = determineTypeFlag(value, typeFlag, typeFlags);
        updateGUI(typeFlag);

        if (!value.isEmpty()) {
            if (typeFlag == KBibTeX::tfSource) {
                /// simple case: field's value is to be shown as BibTeX code, including surrounding curly braces
                FileExporterBibTeX exporter(parent);
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
                                    qCWarning(LOG_KBIBTEX_GUI) << "Could not reset: " << typeFlag << "(" << (typeFlag == KBibTeX::tfSource ? "Source" : (typeFlag == KBibTeX::tfReference ? "Reference" : (typeFlag == KBibTeX::tfPerson ? "Person" : (typeFlag == KBibTeX::tfPlainText ? "PlainText" : (typeFlag == KBibTeX::tfKeyword ? "Keyword" : (typeFlag == KBibTeX::tfVerbatim ? "Verbatim" : "???")))))) << ") " << (typeFlags.testFlag(KBibTeX::tfPerson) ? "Person" : "") << (typeFlags.testFlag(KBibTeX::tfPlainText) ? "PlainText" : "") << (typeFlags.testFlag(KBibTeX::tfReference) ? "Reference" : "") << (typeFlags.testFlag(KBibTeX::tfVerbatim) ? "Verbatim" : "") << " " << typeid((void)*first).name() << " : " << PlainTextValue::text(value);
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

    bool apply(Value &value) const {
        value.clear();
        /// Remove unnecessary white space from input
        /// Exception: source and verbatim content is kept unmodified
        const QString text = typeFlag == KBibTeX::tfSource || typeFlag == KBibTeX::tfVerbatim ? parent->text() : parent->text().simplified();
        if (text.isEmpty())
            return true;

        const EncoderLaTeX &encoder = EncoderLaTeX::instance();
        const QString encodedText = encoder.decode(text);
        static const QRegularExpression invalidCharsForReferenceRegExp(QStringLiteral("[^-_:/a-zA-Z0-9]"));
        if (encodedText.isEmpty())
            return true;
        else if (typeFlag == KBibTeX::tfPlainText) {
            value.append(QSharedPointer<PlainText>(new PlainText(encodedText)));
            return true;
        } else if (typeFlag == KBibTeX::tfReference && !encodedText.contains(invalidCharsForReferenceRegExp)) {
            value.append(QSharedPointer<MacroKey>(new MacroKey(encodedText)));
            return true;
        } else if (typeFlag == KBibTeX::tfPerson) {
            QSharedPointer<Person> person = FileImporterBibTeX::personFromString(encodedText);
            if (!person.isNull())
                value.append(person);
            return true;
        } else if (typeFlag == KBibTeX::tfKeyword) {
            const QList<QSharedPointer<Keyword> > keywords = FileImporterBibTeX::splitKeywords(encodedText);
            for (const auto &keyword : keywords)
                value.append(keyword);
            return true;
        } else if (typeFlag == KBibTeX::tfSource) {
            const QString key = typeFlags.testFlag(KBibTeX::tfPerson) ? QStringLiteral("author") : QStringLiteral("title");
            FileImporterBibTeX importer(parent);
            const QString fakeBibTeXFile = QString(QStringLiteral("@article{dummy, %1=%2}")).arg(key, encodedText);

            const QScopedPointer<const File> file(importer.fromString(fakeBibTeXFile));
            if (!file.isNull() && file->count() == 1) {
                QSharedPointer<Entry> entry = file->first().dynamicCast<Entry>();
                if (!entry.isNull()) {
                    value = entry->value(key);
                    return !value.isEmpty();
                } else
                    qCWarning(LOG_KBIBTEX_GUI) << "Parsing " << fakeBibTeXFile << " did not result in valid entry";
            }
        } else if (typeFlag == KBibTeX::tfVerbatim) {
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(text)));
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
                const KBibTeX::TypeFlag flag = static_cast<KBibTeX::TypeFlag>(p);
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

        const QSharedPointer<ValueItem> first = value.first();
        if (value.count() > 1)
            return typeFlag == KBibTeX::tfSource;
        else if (typeFlag == KBibTeX::tfKeyword && Keyword::isKeyword(*first))
            return true;
        else if (typeFlag == KBibTeX::tfPerson && Person::isPerson(*first))
            return true;
        else if (typeFlag == KBibTeX::tfPlainText && PlainText::isPlainText(*first))
            return true;
        else if (typeFlag == KBibTeX::tfReference && MacroKey::isMacroKey(*first))
            return true;
        else if (typeFlag == KBibTeX::tfVerbatim && VerbatimText::isVerbatimText(*first))
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

    QIcon iconForTypeFlag(KBibTeX::TypeFlag typeFlag) {
        switch (typeFlag) {
        case KBibTeX::tfPlainText: return QIcon::fromTheme(QStringLiteral("draw-text"));
        case KBibTeX::tfReference: return QIcon::fromTheme(QStringLiteral("emblem-symbolic-link"));
        case KBibTeX::tfPerson: return QIcon::fromTheme(QStringLiteral("user-identity"));
        case KBibTeX::tfKeyword: return QIcon::fromTheme(QStringLiteral("edit-find"));
        case KBibTeX::tfSource: return QIcon::fromTheme(QStringLiteral("code-context"));
        case KBibTeX::tfVerbatim: return QIcon::fromTheme(QStringLiteral("preferences-desktop-keyboard"));
        default: return QIcon();
        };
    }

    void updateGUI(KBibTeX::TypeFlag typeFlag) {
        parent->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
        parent->setIcon(iconForTypeFlag(typeFlag));
        switch (typeFlag) {
        case KBibTeX::tfPlainText: parent->setButtonToolTip(i18n("Plain Text")); break;
        case KBibTeX::tfReference: parent->setButtonToolTip(i18n("Reference")); break;
        case KBibTeX::tfPerson: parent->setButtonToolTip(i18n("Person")); break;
        case KBibTeX::tfKeyword: parent->setButtonToolTip(i18n("Keyword")); break;
        case KBibTeX::tfSource:
            parent->setButtonToolTip(i18n("Source Code"));
            parent->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
            break;
        case KBibTeX::tfVerbatim: parent->setButtonToolTip(i18n("Verbatim Text")); break;
        default: parent->setButtonToolTip(QStringLiteral("")); break;
        };
    }

    void openUrl() {
        if (urlToOpen.isValid()) {
            /// Guess mime type for url to open
            QMimeType mimeType = FileInfo::mimeTypeForUrl(urlToOpen);
            const QString mimeTypeName = mimeType.name();
            /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < 0x051f00 // < 5.31.0
            KRun::runUrl(urlToOpen, mimeTypeName, parent, false, false);
#else // KIO_VERSION < 0x051f00 // >= 5.31.0
            KRun::runUrl(urlToOpen, mimeTypeName, parent, KRun::RunFlags());
#endif // KIO_VERSION < 0x051f00
        }
    }

    bool convertValueType(Value &value, KBibTeX::TypeFlag destType) {
        if (value.isEmpty()) return true; /// simple case
        if (destType == KBibTeX::tfSource) return true; /// simple case

        bool result = true;
        const EncoderLaTeX &encoder = EncoderLaTeX::instance();
        QString rawText;
        const QSharedPointer<ValueItem> first = value.first();

        const QSharedPointer<PlainText> plainText = first.dynamicCast<PlainText>();
        if (!plainText.isNull())
            rawText = encoder.encode(plainText->text(), Encoder::TargetEncodingASCII);
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
                        rawText = encoder.encode(QString(QStringLiteral("%1 %2")).arg(person->firstName(), person->lastName()), Encoder::TargetEncodingASCII); // FIXME proper name conversion
                    else {
                        const QSharedPointer<Keyword> keyword = first.dynamicCast<Keyword>();
                        if (!keyword.isNull())
                            rawText = encoder.encode(keyword->text(), Encoder::TargetEncodingASCII);
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
            value.append(QSharedPointer<PlainText>(new PlainText(encoder.decode(rawText))));
            break;
        case KBibTeX::tfVerbatim:
            value.clear();
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(rawText)));
            break;
        case KBibTeX::tfPerson:
            value.clear();
            value.append(QSharedPointer<Person>(FileImporterBibTeX::splitName(encoder.decode(rawText))));
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
            value.append(QSharedPointer<Keyword>(new Keyword(encoder.decode(rawText))));
            break;
        default: {
            // TODO
            result = false;
        }
        }

        return result;
    }

    void updateURL(const QString &text) {
        QSet<QUrl> urls;
        FileInfo::urlsInText(text, FileInfo::TestExistenceYes, file != nullptr && file->property(File::Url).toUrl().isValid() ? QUrl(file->property(File::Url).toUrl()).path() : QString(), urls);
        QSet<QUrl>::ConstIterator urlsIt = urls.constBegin();
        if (urlsIt != urls.constEnd() && (*urlsIt).isValid())
            urlToOpen = (*urlsIt);
        else
            urlToOpen = QUrl();

        /// set special "open URL" button visible if URL (or file or DOI) found
        buttonOpenUrl->setVisible(urlToOpen.isValid());
        buttonOpenUrl->setToolTip(i18n("Open '%1'", urlToOpen.url(QUrl::PreferLocalFile)));
    }

    void textChanged(const QString &text) {
        updateURL(text);
    }
};

FieldLineEdit::FieldLineEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, bool isMultiLine, QWidget *parent)
        : MenuLineEdit(isMultiLine, parent), d(new FieldLineEdit::FieldLineEditPrivate(preferredTypeFlag, typeFlags, this))
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    setObjectName(QStringLiteral("FieldLineEdit"));
    setMenu(d->menuTypes);
    setChildAcceptDrops(false);
    setAcceptDrops(true);
}

FieldLineEdit::~FieldLineEdit()
{
    delete d;
}

bool FieldLineEdit::apply(Value &value) const
{
    return d->apply(value);
}

bool FieldLineEdit::reset(const Value &value)
{
    return d->reset(value);
}

void FieldLineEdit::setReadOnly(bool isReadOnly)
{
    MenuLineEdit::setReadOnly(isReadOnly);
}

void FieldLineEdit::slotTypeChanged(int newTypeFlagInt)
{
    const KBibTeX::TypeFlag newTypeFlag = static_cast<KBibTeX::TypeFlag>(newTypeFlagInt);

    Value value;
    d->apply(value);

    if (d->convertValueType(value, newTypeFlag)) {
        d->typeFlag = newTypeFlag;
        d->reset(value);
    } else
        KMessageBox::error(this, i18n("The current text cannot be used as value of type '%1'.\n\nSwitching back to type '%2'.", BibTeXFields::typeFlagToString(newTypeFlag), BibTeXFields::typeFlagToString(d->typeFlag)));
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
    if (event->mimeData()->hasFormat(QStringLiteral("text/plain")) || event->mimeData()->hasFormat(QStringLiteral("text/x-bibtex")))
        event->acceptProposedAction();
}

void FieldLineEdit::dropEvent(QDropEvent *event)
{
    const QString clipboardText = event->mimeData()->text();
    if (clipboardText.isEmpty()) return;

    bool success = false;
    if (!d->fieldKey.isEmpty() && clipboardText.startsWith(QStringLiteral("@"))) {
        FileImporterBibTeX importer(this);
        QScopedPointer<File> file(importer.fromString(clipboardText));
        const QSharedPointer<Entry> entry = (!file.isNull() && file->count() == 1) ? file->first().dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull() && d->fieldKey == Entry::ftCrossRef) {
            /// handle drop on crossref line differently (use dropped entry's id)
            Value v;
            v.append(QSharedPointer<VerbatimText>(new VerbatimText(entry->id())));
            reset(v);
            emit textChanged(entry->id());
            success = true;
        } else if (!entry.isNull() && entry->contains(d->fieldKey)) {
            /// case for "normal" fields like for journal, pages, ...
            reset(entry->value(d->fieldKey));
            emit textChanged(text());
            success = true;
        }
    }

    if (!success) {
        /// fall-back case: just copy whole text into edit widget
        setText(clipboardText);
        emit textChanged(clipboardText);
    }
}
