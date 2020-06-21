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

#include "fieldlistedit.h"

#include <typeinfo>

#include <QApplication>
#include <QClipboard>
#include <QScrollArea>
#include <QLayout>
#include <QCheckBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QTimer>
#include <QAction>
#include <QPushButton>
#include <QFontDatabase>
#include <QFileDialog>
#include <QMenu>

#include <KMessageBox>
#include <KLocalizedString>
#include <KIO/CopyJob>
#include <KSharedConfig>
#include <KConfigGroup>

#include <File>
#include <Entry>
#include <FileImporterBibTeX>
#include <FileExporterBibTeX>
#include <FileInfo>
#include <AssociatedFiles>
#include "fieldlineedit.h"
#include "element/associatedfilesui.h"
#include "logging_gui.h"

class FieldListEdit::FieldListEditProtected
{
private:
    FieldListEdit *p;
    const int innerSpacing;
    QVBoxLayout *layout;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;

public:
    QList<FieldLineEdit *> lineEditList;
    QWidget *pushButtonContainer;
    QBoxLayout *pushButtonContainerLayout;
    QPushButton *addLineButton;
    const File *file;
    QString fieldKey;
    QWidget *container;
    QScrollArea *scrollArea;
    bool m_isReadOnly;
    QStringList completionItems;

    FieldListEditProtected(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldListEdit *parent)
            : p(parent), innerSpacing(4), preferredTypeFlag(ptf), typeFlags(tf), file(nullptr), m_isReadOnly(false) {
        setupGUI();
    }

    FieldListEditProtected(const FieldListEditProtected &other) = delete;
    FieldListEditProtected &operator= (const FieldListEditProtected &other) = delete;

    void setupGUI() {
        QBoxLayout *outerLayout = new QVBoxLayout(p);
        outerLayout->setMargin(0);
        outerLayout->setSpacing(0);
        scrollArea = new QScrollArea(p);
        outerLayout->addWidget(scrollArea);

        container = new QWidget(scrollArea->viewport());
        container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        scrollArea->setWidget(container);
        layout = new QVBoxLayout(container);
        layout->setMargin(0);
        layout->setSpacing(innerSpacing);

        pushButtonContainer = new QWidget(container);
        pushButtonContainerLayout = new QHBoxLayout(pushButtonContainer);
        pushButtonContainerLayout->setMargin(0);
        layout->addWidget(pushButtonContainer);

        addLineButton = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add"), pushButtonContainer);
        addLineButton->setObjectName(QStringLiteral("addButton"));
        connect(addLineButton, &QPushButton::clicked, p, static_cast<void(FieldListEdit::*)()>(&FieldListEdit::lineAdd));
        connect(addLineButton, &QPushButton::clicked, p, &FieldListEdit::modified);
        pushButtonContainerLayout->addWidget(addLineButton);

        layout->addStretch(100);

        scrollArea->setBackgroundRole(QPalette::Base);
        scrollArea->ensureWidgetVisible(container);
        scrollArea->setWidgetResizable(true);
    }

    void addButton(QPushButton *button) {
        button->setParent(pushButtonContainer);
        pushButtonContainerLayout->addWidget(button);
    }

    int recommendedHeight() {
        int heightHint = 0;

        for (const auto *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(lineEditList))
            heightHint += fieldLineEdit->sizeHint().height();

        heightHint += lineEditList.count() * innerSpacing;
        heightHint += addLineButton->sizeHint().height();

        return heightHint;
    }

    FieldLineEdit *addFieldLineEdit() {
        FieldLineEdit *le = new FieldLineEdit(preferredTypeFlag, typeFlags, false, container);
        le->setFile(file);
        le->setAcceptDrops(false);
        le->setReadOnly(m_isReadOnly);
        le->setInnerWidgetsTransparency(true);
        layout->insertWidget(layout->count() - 2, le);
        lineEditList.append(le);

        QPushButton *remove = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), QString(), le);
        remove->setToolTip(i18n("Remove value"));
        le->appendWidget(remove);
        connect(remove, &QPushButton::clicked, p, [this, le]() {
            removeFieldLineEdit(le);
            const QSize size(container->width(), recommendedHeight());
            container->resize(size);
            /// Instead of an 'emit' ...
            QMetaObject::invokeMethod(p, "modified", Qt::DirectConnection, QGenericReturnArgument());
        });

        QPushButton *goDown = new QPushButton(QIcon::fromTheme(QStringLiteral("go-down")), QString(), le);
        goDown->setToolTip(i18n("Move value down"));
        le->appendWidget(goDown);
        connect(goDown, &QPushButton::clicked, p, [this, le]() {
            const bool gotModified = goDownFieldLineEdit(le);
            if (gotModified) {
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(p, "modified", Qt::DirectConnection, QGenericReturnArgument());
            }
        });

        QPushButton *goUp = new QPushButton(QIcon::fromTheme(QStringLiteral("go-up")), QString(), le);
        goUp->setToolTip(i18n("Move value up"));
        le->appendWidget(goUp);
        connect(goUp, &QPushButton::clicked, p, [this, le]() {
            const bool gotModified = goUpFieldLineEdit(le);
            if (gotModified) {
                /// Instead of an 'emit' ...
                QMetaObject::invokeMethod(p, "modified", Qt::DirectConnection, QGenericReturnArgument());
            }
        });

        connect(le, &FieldLineEdit::modified, p, &FieldListEdit::modified);

        return le;
    }

    void removeAllFieldLineEdits() {
        while (!lineEditList.isEmpty()) {
            FieldLineEdit *fieldLineEdit = *lineEditList.begin();
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeFirst();
            delete fieldLineEdit;
        }

        /// This fixes a layout problem where the container element
        /// does not shrink correctly once the line edits have been
        /// removed
        QSize pSize = container->size();
        pSize.setHeight(addLineButton->height());
        container->resize(pSize);
    }

    void removeFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        lineEditList.removeOne(fieldLineEdit);
        layout->removeWidget(fieldLineEdit);
        delete fieldLineEdit;
    }

    bool goDownFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx < lineEditList.count() - 1) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx + 1, fieldLineEdit);
            layout->insertWidget(idx + 1, fieldLineEdit);
            return true; ///< return 'true' upon actual modification
        }
        return false; ///< return 'false' if nothing changed, i.e. FieldLineEdit is already at bottom
    }

    bool goUpFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx > 0) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx - 1, fieldLineEdit);
            layout->insertWidget(idx - 1, fieldLineEdit);
            return true; ///< return 'true' upon actual modification
        }
        return false; ///< return 'false' if nothing changed, i.e. FieldLineEdit is already at top
    }
};

FieldListEdit::FieldListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : QWidget(parent), d(new FieldListEditProtected(preferredTypeFlag, typeFlags, this))
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setMinimumSize(fontMetrics().averageCharWidth() * 30, fontMetrics().averageCharWidth() * 10);
    setAcceptDrops(true);
}

FieldListEdit::~FieldListEdit()
{
    delete d;
}

bool FieldListEdit::reset(const Value &value)
{
    d->removeAllFieldLineEdits();
    for (const auto &valueItem : value) {
        Value v;
        v.append(valueItem);
        FieldLineEdit *fieldLineEdit = addFieldLineEdit();
        fieldLineEdit->setFile(d->file);
        fieldLineEdit->reset(v);
    }
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);

    return true;
}

bool FieldListEdit::apply(Value &value) const
{
    value.clear();

    for (const auto *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(d->lineEditList)) {
        Value v;
        fieldLineEdit->apply(v);
        for (const auto &valueItem : const_cast<const Value &>(v))
            value.append(valueItem);
    }

    return true;
}

bool FieldListEdit::validate(QWidget **widgetWithIssue, QString &message) const
{
    for (const auto *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(d->lineEditList)) {
        const bool v = fieldLineEdit->validate(widgetWithIssue, message);
        if (!v) return false;
    }
    return true;
}

void FieldListEdit::clear()
{
    d->removeAllFieldLineEdits();
}

void FieldListEdit::setReadOnly(bool isReadOnly)
{
    d->m_isReadOnly = isReadOnly;
    for (FieldLineEdit *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(d->lineEditList))
        fieldLineEdit->setReadOnly(isReadOnly);
    d->addLineButton->setEnabled(!isReadOnly);
}

void FieldListEdit::setFile(const File *file)
{
    d->file = file;
    for (FieldLineEdit *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(d->lineEditList))
        fieldLineEdit->setFile(file);
}

void FieldListEdit::setElement(const Element *element)
{
    m_element = element;
    for (FieldLineEdit *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(d->lineEditList))
        fieldLineEdit->setElement(element);
}

void FieldListEdit::setFieldKey(const QString &fieldKey)
{
    d->fieldKey = fieldKey;
    for (FieldLineEdit *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(d->lineEditList))
        fieldLineEdit->setFieldKey(fieldKey);
}

void FieldListEdit::setCompletionItems(const QStringList &items)
{
    d->completionItems = items;
    for (FieldLineEdit *fieldLineEdit : const_cast<const QList<FieldLineEdit *> &>(d->lineEditList))
        fieldLineEdit->setCompletionItems(items);
}

FieldLineEdit *FieldListEdit::addFieldLineEdit()
{
    return d->addFieldLineEdit();
}

void FieldListEdit::addButton(QPushButton *button)
{
    d->addButton(button);
}

void FieldListEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(QStringLiteral("text/plain")) || event->mimeData()->hasFormat(QStringLiteral("text/x-bibtex")))
        event->acceptProposedAction();
}

void FieldListEdit::dropEvent(QDropEvent *event)
{
    const QString clipboardText = QString::fromUtf8(event->mimeData()->data(QStringLiteral("text/plain")));
    event->acceptProposedAction();
    if (clipboardText.isEmpty()) return;

    bool success = false;
    if (!d->fieldKey.isEmpty() && clipboardText.startsWith(QStringLiteral("@"))) {
        FileImporterBibTeX importer(this);
        QScopedPointer<const File> file(importer.fromString(clipboardText));
        const QSharedPointer<Entry> entry = (!file.isNull() && file->count() == 1) ? file->first().dynamicCast<Entry>() : QSharedPointer<Entry>();

        if (!entry.isNull() && d->fieldKey == QStringLiteral("^external")) {
            /// handle "external" list differently
            const auto urlList = FileInfo::entryUrls(entry, QUrl(file->property(File::Url).toUrl()), FileInfo::TestExistence::No);
            Value v;
            v.reserve(urlList.size());
            for (const QUrl &url : urlList) {
                v.append(QSharedPointer<VerbatimText>(new VerbatimText(url.url(QUrl::PreferLocalFile))));
            }
            reset(v);
            emit modified();
            success = true;
        } else if (!entry.isNull() && entry->contains(d->fieldKey)) {
            /// case for "normal" lists like for authors, editors, ...
            reset(entry->value(d->fieldKey));
            emit modified();
            success = true;
        }
    }

    if (!success) {
        /// In case above cases were not met and thus 'success' is still false,
        /// clear this list edit and use the clipboad text as its single and only list element
        d->removeAllFieldLineEdits();
        FieldLineEdit *fle = addFieldLineEdit();
        fle->setText(clipboardText);
        emit modified();
    }
}

void FieldListEdit::lineAdd(Value *value)
{
    FieldLineEdit *le = addFieldLineEdit();
    le->setCompletionItems(d->completionItems);
    if (value != nullptr)
        le->reset(*value);
}

void FieldListEdit::lineAdd()
{
    FieldLineEdit *newEdit = addFieldLineEdit();
    newEdit->setCompletionItems(d->completionItems);
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
    newEdit->setFocus(Qt::ShortcutFocusReason);
}

PersonListEdit::PersonListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : FieldListEdit(preferredTypeFlag, typeFlags, parent)
{
    m_checkBoxOthers = new QCheckBox(i18n("... and others (et al.)"), this);
    connect(m_checkBoxOthers, &QCheckBox::toggled, this, &PersonListEdit::modified);
    QBoxLayout *boxLayout = static_cast<QBoxLayout *>(layout());
    boxLayout->addWidget(m_checkBoxOthers);

    m_buttonAddNamesFromClipboard = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-paste")), i18n("Add from Clipboard"), this);
    m_buttonAddNamesFromClipboard->setToolTip(i18n("Add a list of names from clipboard"));
    addButton(m_buttonAddNamesFromClipboard);

    connect(m_buttonAddNamesFromClipboard, &QPushButton::clicked, this, &PersonListEdit::slotAddNamesFromClipboard);
}

bool PersonListEdit::reset(const Value &value)
{
    Value internal = value;

    m_checkBoxOthers->setCheckState(Qt::Unchecked);
    QSharedPointer<PlainText> pt;
    if (!internal.isEmpty() && !(pt = internal.last().dynamicCast<PlainText>()).isNull()) {
        if (pt->text() == QStringLiteral("others")) {
            internal.erase(internal.end() - 1);
            m_checkBoxOthers->setCheckState(Qt::Checked);
        }
    }

    return FieldListEdit::reset(internal);
}

bool PersonListEdit::apply(Value &value) const
{
    bool result = FieldListEdit::apply(value);

    if (result && m_checkBoxOthers->checkState() == Qt::Checked)
        value.append(QSharedPointer<PlainText>(new PlainText(QStringLiteral("others"))));

    return result;
}

void PersonListEdit::setReadOnly(bool isReadOnly)
{
    FieldListEdit::setReadOnly(isReadOnly);
    m_checkBoxOthers->setEnabled(!isReadOnly);
    m_buttonAddNamesFromClipboard->setEnabled(!isReadOnly);
}

void PersonListEdit::slotAddNamesFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString text = clipboard->text(QClipboard::Clipboard);
    if (text.isEmpty())
        text = clipboard->text(QClipboard::Selection);
    if (!text.isEmpty()) {
        const QList<QSharedPointer<Person> > personList = FileImporterBibTeX::splitNames(text);
        for (const QSharedPointer<Person> &person : personList) {
            Value *value = new Value();
            value->append(person);
            lineAdd(value);
            delete value;
        }
        if (!personList.isEmpty())
            emit modified();
    }
}


UrlListEdit::UrlListEdit(QWidget *parent)
        : FieldListEdit(KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, parent)
{
    m_buttonAddFile = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add file..."), this);
    addButton(m_buttonAddFile);
    QMenu *menuAddFile = new QMenu(m_buttonAddFile);
    m_buttonAddFile->setMenu(menuAddFile);
    connect(m_buttonAddFile, &QPushButton::clicked, m_buttonAddFile, &QPushButton::showMenu);

    menuAddFile->addAction(QIcon::fromTheme(QStringLiteral("emblem-symbolic-link")), i18n("Add reference ..."), this, [this]() {
        slotAddReference();
    });
    menuAddFile->addAction(QIcon::fromTheme(QStringLiteral("emblem-symbolic-link")), i18n("Add reference from clipboard"), this, [this]() {
        slotAddReferenceFromClipboard();
    });
}

void UrlListEdit::slotAddReference()
{
    QUrl bibtexUrl(d->file != nullptr ? d->file->property(File::Url, QVariant()).toUrl() : QUrl());
    if (bibtexUrl.isValid()) {
        const QFileInfo fi(bibtexUrl.path());
        bibtexUrl.setPath(fi.absolutePath());
    }
    const QUrl documentUrl = QFileDialog::getOpenFileUrl(this, i18n("File to Associate"), bibtexUrl);
    if (documentUrl.isValid())
        addReference(documentUrl);
}

void UrlListEdit::slotAddReferenceFromClipboard()
{
    const QUrl url = QUrl::fromUserInput(QApplication::clipboard()->text());
    if (url.isValid())
        addReference(url);
}

void UrlListEdit::addReference(const QUrl &url) {
    const Entry *entry = dynamic_cast<const Entry *>(m_element);
    if (entry != nullptr) {
        QSharedPointer<Entry> fakeTempEntry(new Entry(entry->type(), entry->id()));
        const QString visibleFilename = AssociatedFilesUI::associateUrl(url, fakeTempEntry, d->file, false, this);
        if (!visibleFilename.isEmpty()) {
            Value *value = new Value();
            value->append(QSharedPointer<VerbatimText>(new VerbatimText(visibleFilename)));
            lineAdd(value);
            delete value;
            emit modified();
        }
    }
}

void UrlListEdit::downloadAndSaveLocally(const QUrl &url)
{
    /// Only proceed if Url is valid and points to a remote location
    if (url.isValid() && !url.isLocalFile()) {
        /// Get filename from url (without any path/directory part)
        QString filename = url.fileName();
        /// Build QFileInfo from current BibTeX file if available
        QFileInfo bibFileinfo = d->file != nullptr ? QFileInfo(d->file->property(File::Url).toUrl().path()) : QFileInfo();
        /// Build proposal to a local filename for remote file
        filename = bibFileinfo.isFile() ? bibFileinfo.absolutePath() + QDir::separator() + filename : filename;
        /// Ask user for actual local filename to save remote file to
        filename = QFileDialog::getSaveFileName(this, i18n("Save file locally"), filename, QStringLiteral("application/pdf application/postscript image/vnd.djvu"));
        /// Check if user entered a valid filename ...
        if (!filename.isEmpty()) {
            /// Ask user if reference to local file should be
            /// relative or absolute in relation to the BibTeX file
            const QString absoluteFilename = filename;
            QString visibleFilename = filename;
            if (bibFileinfo.isFile())
                visibleFilename = askRelativeOrStaticFilename(this, absoluteFilename, d->file->property(File::Url).toUrl());

            /// Download remote file and save it locally
            setEnabled(false);
            setCursor(Qt::WaitCursor);
            KIO::CopyJob *job = KIO::copy(url, QUrl::fromLocalFile(absoluteFilename), KIO::Overwrite);
            job->setProperty("visibleFilename", QVariant::fromValue<QString>(visibleFilename));
            connect(job, &KJob::result, this, &UrlListEdit::downloadFinished);
        }
    }
}

void UrlListEdit::downloadFinished(KJob *j) {
    KIO::CopyJob *job = static_cast<KIO::CopyJob *>(j);
    if (job->error() == 0) {
        /// Download succeeded, add reference to local file to this BibTeX entry
        Value *value = new Value();
        value->append(QSharedPointer<VerbatimText>(new VerbatimText(job->property("visibleFilename").toString())));
        lineAdd(value);
        delete value;
    } else {
        qCWarning(LOG_KBIBTEX_GUI) << "Downloading" << (*job->srcUrls().constBegin()).toDisplayString() << "failed with error" << job->error() << job->errorString();
    }
    setEnabled(true);
    unsetCursor();
}

void UrlListEdit::textChanged(QPushButton *buttonSaveLocally, FieldLineEdit *fieldLineEdit)
{
    if (buttonSaveLocally == nullptr || fieldLineEdit == nullptr) return; ///< should never happen!

    /// Create URL from new text to make some tests on it
    /// Only remote URLs are of interest, therefore no tests
    /// on local file or relative paths
    const QString newText = fieldLineEdit->text();
    const QString lowerText = newText.toLower();

    /// Enable button only if Url is valid and points to a remote
    /// DjVu, PDF, or PostScript file
    // TODO more file types?
    const bool canBeSaved = lowerText.contains(QStringLiteral("://")) && (lowerText.endsWith(QStringLiteral(".djvu")) || lowerText.endsWith(QStringLiteral(".pdf")) || lowerText.endsWith(QStringLiteral(".ps")));
    buttonSaveLocally->setEnabled(canBeSaved);
    buttonSaveLocally->setToolTip(canBeSaved ? i18n("Save file '%1' locally", newText) : QString());
}

QString UrlListEdit::askRelativeOrStaticFilename(QWidget *parent, const QString &absoluteFilename, const QUrl &baseUrl)
{
    QFileInfo baseUrlInfo = baseUrl.isValid() ? QFileInfo(baseUrl.path()) : QFileInfo();
    QFileInfo filenameInfo(absoluteFilename);
    if (baseUrl.isValid() && (filenameInfo.absolutePath() == baseUrlInfo.absolutePath() || filenameInfo.absolutePath().startsWith(baseUrlInfo.absolutePath() + QDir::separator()))) {
        // TODO cover level-up cases like "../../test.pdf"
        const QString relativePath = filenameInfo.absolutePath().mid(baseUrlInfo.absolutePath().length() + 1);
        const QString relativeFilename = relativePath + (relativePath.isEmpty() ? QString() : QString(QDir::separator())) + filenameInfo.fileName();
        if (KMessageBox::questionYesNo(parent, i18n("<qt><p>Use a filename relative to the bibliography file?</p><p>The relative path would be<br/><tt style=\"font-family: %3;\">%1</tt></p><p>The absolute path would be<br/><tt style=\"font-family: %3;\">%2</tt></p></qt>", relativeFilename, absoluteFilename, QFontDatabase::systemFont(QFontDatabase::FixedFont).family()), i18n("Relative Path"), KGuiItem(i18n("Relative Path")), KGuiItem(i18n("Absolute Path"))) == KMessageBox::Yes)
            return relativeFilename;
    }
    return absoluteFilename;
}

FieldLineEdit *UrlListEdit::addFieldLineEdit()
{
    /// Call original implementation to get an instance of a FieldLineEdit
    FieldLineEdit *fieldLineEdit = FieldListEdit::addFieldLineEdit();

    /// Create a new "save locally" button
    QPushButton *buttonSaveLocally = new QPushButton(QIcon::fromTheme(QStringLiteral("document-save")), QString(), fieldLineEdit);
    buttonSaveLocally->setToolTip(i18n("Save file locally"));
    buttonSaveLocally->setEnabled(false);
    /// Append button to new FieldLineEdit
    fieldLineEdit->appendWidget(buttonSaveLocally);
    /// Connect signals to react on button events
    /// or changes in the FieldLineEdit's text
    connect(buttonSaveLocally, &QPushButton::clicked, this, [this, fieldLineEdit]() {
        downloadAndSaveLocally(QUrl::fromUserInput(fieldLineEdit->text()));
    });
    connect(fieldLineEdit, &FieldLineEdit::textChanged, this, [this, buttonSaveLocally, fieldLineEdit]() {
        textChanged(buttonSaveLocally, fieldLineEdit);
    });

    return fieldLineEdit;
}

void UrlListEdit::setReadOnly(bool isReadOnly)
{
    FieldListEdit::setReadOnly(isReadOnly);
    m_buttonAddFile->setEnabled(!isReadOnly);
}


const QString KeywordListEdit::keyGlobalKeywordList = QStringLiteral("globalKeywordList");

KeywordListEdit::KeywordListEdit(QWidget *parent)
        : FieldListEdit(KBibTeX::TypeFlag::Keyword, KBibTeX::TypeFlag::Keyword | KBibTeX::TypeFlag::Source, parent), m_config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), m_configGroupName(QStringLiteral("Global Keywords"))
{
    m_buttonAddKeywordsFromList = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), i18n("Add Keywords from List"), this);
    m_buttonAddKeywordsFromList->setToolTip(i18n("Add keywords as selected from a pre-defined list of keywords"));
    addButton(m_buttonAddKeywordsFromList);
    connect(m_buttonAddKeywordsFromList, &QPushButton::clicked, this, &KeywordListEdit::slotAddKeywordsFromList);
    m_buttonAddKeywordsFromClipboard = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-paste")), i18n("Add Keywords from Clipboard"), this);
    m_buttonAddKeywordsFromClipboard->setToolTip(i18n("Add a punctuation-separated list of keywords from clipboard"));
    addButton(m_buttonAddKeywordsFromClipboard);
    connect(m_buttonAddKeywordsFromClipboard, &QPushButton::clicked, this, &KeywordListEdit::slotAddKeywordsFromClipboard);
}

void KeywordListEdit::slotAddKeywordsFromList()
{
    /// fetch stored, global keywords
    KConfigGroup configGroup(m_config, m_configGroupName);
    QStringList keywords = configGroup.readEntry(KeywordListEdit::keyGlobalKeywordList, QStringList());

    /// use a map for case-insensitive sorting of strings
    /// (recommended by Qt's documentation)
    QMap<QString, QString> forCaseInsensitiveSorting;
    /// insert all stored, global keywords
    for (const QString &keyword : const_cast<const QStringList &>(keywords))
        forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// insert all unique keywords used in this file
    for (const QString &keyword : const_cast<const QSet<QString> &>(m_keywordsFromFile))
        forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// re-create string list from map's values
    keywords = forCaseInsensitiveSorting.values();

    // FIXME QInputDialog does not have a 'getItemList'
    /*
    bool ok = false;
    const QStringList newKeywordList = KInputDialog::getItemList(i18n("Add Keywords"), i18n("Select keywords to add:"), keywords, QStringList(), true, &ok, this);
    if (ok) {
        for(const QString &newKeywordText : newKeywordList) {
            Value *value = new Value();
            value->append(QSharedPointer<Keyword>(new Keyword(newKeywordText)));
            lineAdd(value);
            delete value;
        }
        if (!newKeywordList.isEmpty())
            emit modified();
    }
    */
}

void KeywordListEdit::slotAddKeywordsFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString text = clipboard->text(QClipboard::Clipboard);
    if (text.isEmpty()) ///< use "mouse" clipboard as fallback
        text = clipboard->text(QClipboard::Selection);
    if (!text.isEmpty()) {
        const QList<QSharedPointer<Keyword> > keywords = FileImporterBibTeX::splitKeywords(text);
        for (const auto &keyword : keywords) {
            Value *value = new Value();
            value->append(keyword);
            lineAdd(value);
            delete value;
        }
        if (!keywords.isEmpty())
            emit modified();
    }
}

void KeywordListEdit::setReadOnly(bool isReadOnly)
{
    FieldListEdit::setReadOnly(isReadOnly);
    m_buttonAddKeywordsFromList->setEnabled(!isReadOnly);
    m_buttonAddKeywordsFromClipboard->setEnabled(!isReadOnly);
}

void KeywordListEdit::setFile(const File *file)
{
    if (file == nullptr)
        m_keywordsFromFile.clear();
    else
        m_keywordsFromFile = file->uniqueEntryValuesSet(Entry::ftKeywords);

    FieldListEdit::setFile(file);
}

void KeywordListEdit::setCompletionItems(const QStringList &items)
{
    /// fetch stored, global keywords
    KConfigGroup configGroup(m_config, m_configGroupName);
    QStringList keywords = configGroup.readEntry(KeywordListEdit::keyGlobalKeywordList, QStringList());

    /// use a map for case-insensitive sorting of strings
    /// (recommended by Qt's documentation)
    QMap<QString, QString> forCaseInsensitiveSorting;
    /// insert all stored, global keywords
    for (const QString &keyword : const_cast<const QStringList &>(keywords))
        forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// insert all unique keywords used in this file
    for (const QString &keyword : const_cast<const QStringList &>(items))
        forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// re-create string list from map's values
    keywords = forCaseInsensitiveSorting.values();

    FieldListEdit::setCompletionItems(keywords);
}
