/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QBuffer>
#include <QFileInfo>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPushButton>
#include <QFontDatabase>
#include <QUrl>
#include <QMimeType>
#include <QMimeData>
#include <QRegularExpression>

#include <kio_version.h>
#if KIO_VERSION >= 0x054700 // >= 5.71.0
#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#else // < 5.71.0
#include <KRun>
#endif // KIO_VERSION >= 0x054700
#include <KMessageBox>
#include <KLocalizedString>

#include <BibTeXFields>
#include <Preferences>
#include <File>
#include <Entry>
#include <Value>
#include <FileInfo>
#include <FileImporterBibTeX>
#include <FileExporterBibTeX>
#include <EncoderLaTeX>
#include "logging_gui.h"

class FieldLineEdit::FieldLineEditPrivate
{
private:
    FieldLineEdit *parent;
    KBibTeX::TypeFlags typeFlags;
    QPushButton *buttonOpenUrl;

public:
    QMenu *menuTypes;
    const KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlag typeFlag;
    QUrl urlToOpen;
    const File *file;
    QString fieldKey;

    FieldLineEditPrivate(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldLineEdit *p)
            : parent(p), typeFlags(tf), preferredTypeFlag(ptf), file(nullptr) {
        menuTypes = new QMenu(parent);
        setupMenu();

        buttonOpenUrl = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open-remote")), QString(), parent);
        buttonOpenUrl->setVisible(false);
        buttonOpenUrl->setProperty("isConst", true);
        parent->appendWidget(buttonOpenUrl);
        connect(buttonOpenUrl, &QPushButton::clicked, parent, [this]() {
            openUrl();
        });

        connect(p, &FieldLineEdit::textChanged, p, [this](const QString & text) {
            textChanged(text);
        });

        Value value;
        typeFlag = determineTypeFlag(value, preferredTypeFlag, typeFlags);
        updateGUI(typeFlag);
    }

    bool reset(const Value &value, const KBibTeX::TypeFlag preferredTypeFlag) {
        bool result = false;
        QString text;
        typeFlag = determineTypeFlag(value, preferredTypeFlag, typeFlags);
        updateGUI(typeFlag);

        if (!value.isEmpty()) {
            if (typeFlag == KBibTeX::TypeFlag::Source) {
                /// simple case: field's value is to be shown as BibTeX code, including surrounding curly braces
                FileExporterBibTeX exporter(parent);
                text = exporter.valueToBibTeX(value, Encoder::TargetEncoding::UTF8);
                result = true;
            } else {
                /// except for the source view type flag, type flag views do not support composed values,
                /// therefore only the first value will be shown
                const QSharedPointer<ValueItem> first = value.first();

                const QSharedPointer<PlainText> plainText = first.dynamicCast<PlainText>();
                if (typeFlag == KBibTeX::TypeFlag::PlainText && !plainText.isNull()) {
                    text = plainText->text();
                    result = true;
                } else {
                    const QSharedPointer<Person> person = first.dynamicCast<Person>();
                    if (typeFlag == KBibTeX::TypeFlag::Person && !person.isNull()) {
                        text = Person::transcribePersonName(person.data(), Preferences::instance().personNameFormat());
                        result = true;
                    } else {
                        const QSharedPointer<MacroKey> macroKey = first.dynamicCast<MacroKey>();
                        if (typeFlag == KBibTeX::TypeFlag::Reference && !macroKey.isNull()) {
                            text = macroKey->text();
                            result = true;
                        } else {
                            const QSharedPointer<Keyword> keyword = first.dynamicCast<Keyword>();
                            if (typeFlag == KBibTeX::TypeFlag::Keyword && !keyword.isNull()) {
                                text = keyword->text();
                                result = true;
                            } else {
                                const QSharedPointer<VerbatimText> verbatimText = first.dynamicCast<VerbatimText>();
                                if (typeFlag == KBibTeX::TypeFlag::Verbatim && !verbatimText.isNull()) {
                                    text = verbatimText->text();
                                    result = true;
                                } else
                                    qCWarning(LOG_KBIBTEX_GUI) << "Could not reset: " << typeFlag << " " << typeid((void)*first).name() << " : " << PlainTextValue::text(value);
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
        const QString text = typeFlag == KBibTeX::TypeFlag::Source || typeFlag == KBibTeX::TypeFlag::Verbatim ? parent->text() : parent->text().simplified();
        if (text.isEmpty())
            return true;

        const EncoderLaTeX &encoder = EncoderLaTeX::instance();
        const QString encodedText = encoder.decode(text);
        static const QRegularExpression invalidCharsForReferenceRegExp(QStringLiteral("[^-_:/a-zA-Z0-9]"));
        if (encodedText.isEmpty())
            return true;
        else if (typeFlag == KBibTeX::TypeFlag::PlainText) {
            value.append(QSharedPointer<PlainText>(new PlainText(encodedText)));
            return true;
        } else if (typeFlag == KBibTeX::TypeFlag::Reference && !encodedText.contains(invalidCharsForReferenceRegExp)) {
            value.append(QSharedPointer<MacroKey>(new MacroKey(encodedText)));
            return true;
        } else if (typeFlag == KBibTeX::TypeFlag::Person) {
            QSharedPointer<Person> person = FileImporterBibTeX::personFromString(encodedText);
            if (!person.isNull())
                value.append(person);
            return true;
        } else if (typeFlag == KBibTeX::TypeFlag::Keyword) {
            const QList<QSharedPointer<Keyword> > keywords = FileImporterBibTeX::splitKeywords(encodedText);
            for (const auto &keyword : keywords)
                value.append(keyword);
            return true;
        } else if (typeFlag == KBibTeX::TypeFlag::Source) {
            const QString key = typeFlags.testFlag(KBibTeX::TypeFlag::Person) ? QStringLiteral("author") : QStringLiteral("title");
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
        } else if (typeFlag == KBibTeX::TypeFlag::Verbatim) {
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(text)));
            return true;
        }

        return false;
    }

    int validateCurlyBracketContext(const QString &text) const {
        int openingCB = 0, closingCB = 0;

        for (int i = 0; i < text.length(); ++i) {
            if (i == 0 || text[i - 1] != QLatin1Char('\\')) {
                if (text[i] == QLatin1Char('{')) ++openingCB;
                else if (text[i] == QLatin1Char('}')) ++closingCB;
            }
        }

        return openingCB - closingCB;
    }

    bool validate(QWidget **widgetWithIssue, QString &message) const {
        message.clear();

        /// Remove unnecessary white space from input
        /// Exception: source and verbatim content is kept unmodified
        const QString text = typeFlag == KBibTeX::TypeFlag::Source || typeFlag == KBibTeX::TypeFlag::Verbatim ? parent->text() : parent->text().simplified();
        if (text.isEmpty())
            return true;

        const EncoderLaTeX &encoder = EncoderLaTeX::instance();
        const QString encodedText = encoder.decode(text);
        if (encodedText.isEmpty())
            return true;

        bool result = false;
        if (typeFlag == KBibTeX::TypeFlag::PlainText || typeFlag == KBibTeX::TypeFlag::Person || typeFlag == KBibTeX::TypeFlag::Keyword) {
            result = validateCurlyBracketContext(text) == 0;
            if (!result) message = i18n("Opening and closing curly brackets do not match.");
        } else if (typeFlag == KBibTeX::TypeFlag::Reference) {
            static const QRegularExpression validReferenceRegExp(QStringLiteral("^[-_:/a-zA-Z0-9]+$"));
            const QRegularExpressionMatch validReferenceMatch = validReferenceRegExp.match(text);
            result = validReferenceMatch.hasMatch() && validReferenceMatch.captured() == text;
            if (!result) message = i18n("Reference contains characters outside of the allowed set.");
        } else if (typeFlag == KBibTeX::TypeFlag::Source) {
            const QString key = typeFlags.testFlag(KBibTeX::TypeFlag::Person) ? QStringLiteral("author") : QStringLiteral("title");
            FileImporterBibTeX importer(parent);
            const QString fakeBibTeXFile = QString(QStringLiteral("@article{dummy, %1=%2}")).arg(key, encodedText);

            const QScopedPointer<const File> file(importer.fromString(fakeBibTeXFile));
            if (file.isNull() || file->count() != 1) return false;
            QSharedPointer<Entry> entry = file->first().dynamicCast<Entry>();
            result = !entry.isNull() && entry->count() == 1;
            if (!result) message = i18n("Source code could not be parsed correctly.");
        } else if (typeFlag == KBibTeX::TypeFlag::Verbatim) {
            result = validateCurlyBracketContext(text) == 0;
            if (!result) message = i18n("Opening and closing curly brackets do not match.");
        }

        if (!result && widgetWithIssue != nullptr)
            *widgetWithIssue = parent;

        return result;
    }

    void clear() {
        const KBibTeX::TypeFlag newTypeFlag = typeFlags.testFlag(preferredTypeFlag) ? preferredTypeFlag : KBibTeX::TypeFlag::Source;
        if (newTypeFlag != typeFlag)
            updateGUI(typeFlag = newTypeFlag);
    }

    KBibTeX::TypeFlag determineTypeFlag(const Value &value, KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags availableTypeFlags) {
        KBibTeX::TypeFlag result = KBibTeX::TypeFlag::Source;
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
        if (value.isEmpty() || typeFlag == KBibTeX::TypeFlag::Source)
            return true;

        const QSharedPointer<ValueItem> first = value.first();
        if (value.count() > 1)
            return typeFlag == KBibTeX::TypeFlag::Source;
        else if (typeFlag == KBibTeX::TypeFlag::Keyword && Keyword::isKeyword(*first))
            return true;
        else if (typeFlag == KBibTeX::TypeFlag::Person && Person::isPerson(*first))
            return true;
        else if (typeFlag == KBibTeX::TypeFlag::PlainText && PlainText::isPlainText(*first))
            return true;
        else if (typeFlag == KBibTeX::TypeFlag::Reference && MacroKey::isMacroKey(*first))
            return true;
        else if (typeFlag == KBibTeX::TypeFlag::Verbatim && VerbatimText::isVerbatimText(*first))
            return true;
        else
            return false;
    }


    void setupMenu() {
        menuTypes->clear();

        if (typeFlags.testFlag(KBibTeX::TypeFlag::PlainText)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::TypeFlag::PlainText), i18n("Plain Text"));
            connect(action, &QAction::triggered, parent, [this]() {
                typeChanged(KBibTeX::TypeFlag::PlainText);
            });
        }
        if (typeFlags.testFlag(KBibTeX::TypeFlag::Reference)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::TypeFlag::Reference), i18n("Reference"));
            connect(action, &QAction::triggered, parent, [this]() {
                typeChanged(KBibTeX::TypeFlag::Reference);
            });
        }
        if (typeFlags.testFlag(KBibTeX::TypeFlag::Person)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::TypeFlag::Person), i18n("Person"));
            connect(action, &QAction::triggered, parent, [this]() {
                typeChanged(KBibTeX::TypeFlag::Person);
            });
        }
        if (typeFlags.testFlag(KBibTeX::TypeFlag::Keyword)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::TypeFlag::Keyword), i18n("Keyword"));
            connect(action, &QAction::triggered, parent, [this]() {
                typeChanged(KBibTeX::TypeFlag::Keyword);
            });
        }
        if (typeFlags.testFlag(KBibTeX::TypeFlag::Source)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::TypeFlag::Source), i18n("Source Code"));
            connect(action, &QAction::triggered, parent, [this]() {
                typeChanged(KBibTeX::TypeFlag::Source);
            });
        }
        if (typeFlags.testFlag(KBibTeX::TypeFlag::Verbatim)) {
            QAction *action = menuTypes->addAction(iconForTypeFlag(KBibTeX::TypeFlag::Verbatim), i18n("Verbatim Text"));
            connect(action, &QAction::triggered, parent, [this]() {
                typeChanged(KBibTeX::TypeFlag::Verbatim);
            });
        }
    }

    QIcon iconForTypeFlag(KBibTeX::TypeFlag typeFlag) {
        switch (typeFlag) {
        case KBibTeX::TypeFlag::Invalid: return QIcon();
        case KBibTeX::TypeFlag::PlainText: return QIcon::fromTheme(QStringLiteral("draw-text"));
        case KBibTeX::TypeFlag::Reference: return QIcon::fromTheme(QStringLiteral("emblem-symbolic-link"));
        case KBibTeX::TypeFlag::Person: return QIcon::fromTheme(QStringLiteral("user-identity"));
        case KBibTeX::TypeFlag::Keyword: return QIcon::fromTheme(QStringLiteral("edit-find"));
        case KBibTeX::TypeFlag::Source: return QIcon::fromTheme(QStringLiteral("code-context"));
        case KBibTeX::TypeFlag::Verbatim: return QIcon::fromTheme(QStringLiteral("preferences-desktop-keyboard"));
        }
        return QIcon(); //< should never happen as switch above covers all cases
    }

    void updateGUI(KBibTeX::TypeFlag typeFlag) {
        parent->setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
        parent->setIcon(iconForTypeFlag(typeFlag));
        switch (typeFlag) {
        case KBibTeX::TypeFlag::Invalid: parent->setButtonToolTip(QString()); break;
        case KBibTeX::TypeFlag::PlainText: parent->setButtonToolTip(i18n("Plain Text")); break;
        case KBibTeX::TypeFlag::Reference: parent->setButtonToolTip(i18n("Reference")); break;
        case KBibTeX::TypeFlag::Person: parent->setButtonToolTip(i18n("Person")); break;
        case KBibTeX::TypeFlag::Keyword: parent->setButtonToolTip(i18n("Keyword")); break;
        case KBibTeX::TypeFlag::Source:
            parent->setButtonToolTip(i18n("Source Code"));
            parent->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
            break;
        case KBibTeX::TypeFlag::Verbatim: parent->setButtonToolTip(i18n("Verbatim Text")); break;
        }
    }

    void openUrl() {
        if (urlToOpen.isValid()) {
            /// Guess mime type for url to open
            QMimeType mimeType = FileInfo::mimeTypeForUrl(urlToOpen);
            const QString mimeTypeName = mimeType.name();
            /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < 0x054700 // < 5.71.0
            KRun::runUrl(urlToOpen, mimeTypeName, parent, KRun::RunFlags());
#else // KIO_VERSION < 0x054700 // >= 5.71.0
            KIO::OpenUrlJob *job = new KIO::OpenUrlJob(urlToOpen, mimeTypeName);
            job->setUiDelegate(new KIO::JobUiDelegate());
            job->start();
#endif // KIO_VERSION < 0x054700
        }
    }

    bool convertValueType(Value &value, KBibTeX::TypeFlag destType) {
        if (value.isEmpty()) return true; /// simple case
        if (destType == KBibTeX::TypeFlag::Source) return true; /// simple case

        bool result = true;
        const EncoderLaTeX &encoder = EncoderLaTeX::instance();
        QString rawText;
        const QSharedPointer<ValueItem> first = value.first();

        const QSharedPointer<PlainText> plainText = first.dynamicCast<PlainText>();
        if (!plainText.isNull())
            rawText = encoder.encode(plainText->text(), Encoder::TargetEncoding::ASCII);
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
                        rawText = encoder.encode(QString(QStringLiteral("%1 %2")).arg(person->firstName(), person->lastName()), Encoder::TargetEncoding::ASCII); // FIXME proper name conversion
                    else {
                        const QSharedPointer<Keyword> keyword = first.dynamicCast<Keyword>();
                        if (!keyword.isNull())
                            rawText = encoder.encode(keyword->text(), Encoder::TargetEncoding::ASCII);
                        else {
                            // TODO case missed?
                            result = false;
                        }
                    }
                }
            }
        }

        switch (destType) {
        case KBibTeX::TypeFlag::PlainText:
            value.clear();
            value.append(QSharedPointer<PlainText>(new PlainText(encoder.decode(rawText))));
            break;
        case KBibTeX::TypeFlag::Verbatim:
            value.clear();
            value.append(QSharedPointer<VerbatimText>(new VerbatimText(rawText)));
            break;
        case KBibTeX::TypeFlag::Person:
            value.clear();
            value.append(QSharedPointer<Person>(FileImporterBibTeX::splitName(encoder.decode(rawText))));
            break;
        case KBibTeX::TypeFlag::Reference: {
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
        case KBibTeX::TypeFlag::Keyword:
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
        FileInfo::urlsInText(text, FileInfo::TestExistence::Yes, file != nullptr && file->property(File::Url).toUrl().isValid() ? QUrl(file->property(File::Url).toUrl()).path() : QString(), urls);
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

    void typeChanged(const KBibTeX::TypeFlag newTypeFlag)
    {
        Value value;
        apply(value);

        if (convertValueType(value, newTypeFlag)) {
            reset(value, newTypeFlag);
            QMetaObject::invokeMethod(parent, "modified", Qt::DirectConnection, QGenericReturnArgument());
        } else
            KMessageBox::error(parent, i18n("The current text cannot be used as value of type '%1'.\n\nSwitching back to type '%2'.", BibTeXFields::typeFlagToString(newTypeFlag), BibTeXFields::typeFlagToString(typeFlag)));
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
    return d->reset(value, d->preferredTypeFlag);
}

bool FieldLineEdit::validate(QWidget **widgetWithIssue, QString &message) const
{
    return d->validate(widgetWithIssue, message);
}

void FieldLineEdit::clear()
{
    MenuLineEdit::clear();
    d->clear();
}

void FieldLineEdit::setReadOnly(bool isReadOnly)
{
    MenuLineEdit::setReadOnly(isReadOnly);
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

void FieldLineEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(QStringLiteral("text/plain")) || event->mimeData()->hasFormat(QStringLiteral("text/x-bibtex")))
        event->acceptProposedAction();
}

void FieldLineEdit::dropEvent(QDropEvent *event)
{
    const QString clipboardText = QString::fromUtf8(event->mimeData()->data(QStringLiteral("text/plain")));
    event->acceptProposedAction();
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
        /// In case above cases were not met and thus 'success' is still false,
        /// clear this line edit and use the clipboad text as its content
        setText(clipboardText);
        emit textChanged(clipboardText);
    }
}
