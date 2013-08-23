/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <QApplication>
#include <QClipboard>
#include <QScrollArea>
#include <QLayout>
#include <QSignalMapper>
#include <QCheckBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QTimer>

#include <KMessageBox>
#include <KLocale>
#include <KPushButton>
#include <KFileDialog>
#include <KInputDialog>
#include <KIO/NetAccess>
#include <KGlobalSettings>

#include <fileinfo.h>
#include <file.h>
#include <entry.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <fieldlineedit.h>
#include "fieldlistedit.h"

class FieldListEdit::FieldListEditProtected
{
private:
    FieldListEdit *p;
    const int innerSpacing;
    QSignalMapper *smRemove, *smGoUp, *smGoDown;
    QVBoxLayout *layout;
    KBibTeX::TypeFlag preferredTypeFlag;
    KBibTeX::TypeFlags typeFlags;

public:
    QList<FieldLineEdit *> lineEditList;
    QWidget *pushButtonContainer;
    QBoxLayout *pushButtonContainerLayout;
    KPushButton *addLineButton;
    const File *file;
    QString fieldKey;
    QWidget *container;
    QScrollArea *scrollArea;
    bool m_isReadOnly;
    QStringList completionItems;

    FieldListEditProtected(KBibTeX::TypeFlag ptf, KBibTeX::TypeFlags tf, FieldListEdit *parent)
            : p(parent), innerSpacing(4), preferredTypeFlag(ptf), typeFlags(tf), file(NULL), m_isReadOnly(false) {
        smRemove = new QSignalMapper(parent);
        smGoUp = new QSignalMapper(parent);
        smGoDown = new QSignalMapper(parent);
        setupGUI();
    }

    ~FieldListEditProtected() {
        delete smRemove;
        delete smGoUp;
        delete smGoDown;
    }

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

        addLineButton = new KPushButton(KIcon("list-add"), i18n("Add"), pushButtonContainer);
        addLineButton->setObjectName(QLatin1String("addButton"));
        connect(addLineButton, SIGNAL(clicked()), p, SLOT(lineAdd()));
        pushButtonContainerLayout->addWidget(addLineButton);

        layout->addStretch(100);

        connect(smRemove, SIGNAL(mapped(QWidget *)), p, SLOT(lineRemove(QWidget *)));
        connect(smRemove, SIGNAL(mapped(QWidget *)), p, SIGNAL(modified()));
        connect(smGoDown, SIGNAL(mapped(QWidget *)), p, SLOT(lineGoDown(QWidget *)));
        connect(smGoDown, SIGNAL(mapped(QWidget *)), p, SIGNAL(modified()));
        connect(smGoUp, SIGNAL(mapped(QWidget *)), p, SLOT(lineGoUp(QWidget *)));
        connect(smGoDown, SIGNAL(mapped(QWidget *)), p, SIGNAL(modified()));

        scrollArea->setBackgroundRole(QPalette::Base);
        scrollArea->ensureWidgetVisible(container);
        scrollArea->setWidgetResizable(true);
    }

    void addButton(KPushButton *button) {
        button->setParent(pushButtonContainer);
        pushButtonContainerLayout->addWidget(button);
    }

    int recommendedHeight() {
        int heightHint = 0;

        for (QList<FieldLineEdit *>::ConstIterator it = lineEditList.constBegin(); it != lineEditList.constEnd(); ++it)
            heightHint += (*it)->sizeHint().height();

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

        KPushButton *remove = new KPushButton(KIcon("list-remove"), QLatin1String(""), le);
        remove->setToolTip(i18n("Remove value"));
        le->appendWidget(remove);
        connect(remove, SIGNAL(clicked()), smRemove, SLOT(map()));
        smRemove->setMapping(remove, le);

        KPushButton *goDown = new KPushButton(KIcon("go-down"), QLatin1String(""), le);
        goDown->setToolTip(i18n("Move value down"));
        le->appendWidget(goDown);
        connect(goDown, SIGNAL(clicked()), smGoDown, SLOT(map()));
        smGoDown->setMapping(goDown, le);

        KPushButton *goUp = new KPushButton(KIcon("go-up"), QLatin1String(""), le);
        goUp->setToolTip(i18n("Move value up"));
        le->appendWidget(goUp);
        connect(goUp, SIGNAL(clicked()), smGoUp, SLOT(map()));
        smGoUp->setMapping(goUp, le);

        connect(le, SIGNAL(textChanged(QString)), p, SIGNAL(modified()));

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

    void goDownFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx < lineEditList.count() - lineEditList.size()) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx + 1, fieldLineEdit);
            layout->insertWidget(idx + 1, fieldLineEdit);
        }
    }

    void goUpFieldLineEdit(FieldLineEdit *fieldLineEdit) {
        int idx = lineEditList.indexOf(fieldLineEdit);
        if (idx > 0) {
            layout->removeWidget(fieldLineEdit);
            lineEditList.removeOne(fieldLineEdit);
            lineEditList.insert(idx - 1, fieldLineEdit);
            layout->insertWidget(idx - 1, fieldLineEdit);
        }
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
    for (Value::ConstIterator it = value.constBegin(); it != value.constEnd(); ++it) {
        Value v;
        v.append(*it);
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

    for (QList<FieldLineEdit *>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it) {
        Value v;
        (*it)->apply(v);
        for (Value::ConstIterator itv = v.constBegin(); itv != v.constEnd(); ++itv)
            value.append(*itv);
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
    for (QList<FieldLineEdit *>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it)
        (*it)->setReadOnly(isReadOnly);
    d->addLineButton->setEnabled(!isReadOnly);
}

void FieldListEdit::setFile(const File *file)
{
    d->file = file;
    for (QList<FieldLineEdit *>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it)
        (*it)->setFile(file);
}

void FieldListEdit::setElement(const Element *element)
{
    m_element = element;
    for (QList<FieldLineEdit *>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it)
        (*it)->setElement(element);
}

void FieldListEdit::setFieldKey(const QString &fieldKey)
{
    d->fieldKey = fieldKey;
    for (QList<FieldLineEdit *>::ConstIterator it = d->lineEditList.constBegin(); it != d->lineEditList.constEnd(); ++it)
        (*it)->setFieldKey(fieldKey);
}

void FieldListEdit::setCompletionItems(const QStringList &items)
{
    d->completionItems = items;
    for (QList<FieldLineEdit *>::Iterator it = d->lineEditList.begin(); it != d->lineEditList.end(); ++it)
        (*it)->setCompletionItems(items);
}

FieldLineEdit *FieldListEdit::addFieldLineEdit()
{
    return d->addFieldLineEdit();
}

void FieldListEdit::addButton(KPushButton *button)
{
    d->addButton(button);
}

void FieldListEdit::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/plain") || event->mimeData()->hasFormat("text/x-bibtex"))
        event->acceptProposedAction();
}

void FieldListEdit::dropEvent(QDropEvent *event)
{
    const QString clipboardText = event->mimeData()->text();
    if (clipboardText.isEmpty()) return;

    const File *file = NULL;
    if (!d->fieldKey.isEmpty() && clipboardText.startsWith("@")) {
        FileImporterBibTeX importer;
        file = importer.fromString(clipboardText);
        const QSharedPointer<Entry> entry = (file != NULL && file->count() == 1) ? file->first().dynamicCast<Entry>() : QSharedPointer<Entry>();

        if (!entry.isNull() && d->fieldKey == QLatin1String("^external")) {
            /// handle "external" list differently
            QList<KUrl> urlList = FileInfo::entryUrls(entry.data(), KUrl(file->property(File::Url).toUrl()), FileInfo::TestExistanceNo);
            Value v;
            foreach(const KUrl &url, urlList) {
                v.append(QSharedPointer<VerbatimText>(new VerbatimText(url.pathOrUrl())));
            }
            reset(v);
            emit modified();
            return;
        } else if (!entry.isNull() && entry->contains(d->fieldKey)) {
            /// case for "normal" lists like for authors, editors, ...
            reset(entry->value(d->fieldKey));
            emit modified();
            return;
        }
    }

    if (file == NULL || file->count() == 0) {
        /// fall-back case: single field line edit with text
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
    if (value != NULL)
        le->reset(*value);
}

void FieldListEdit::lineAdd()
{
    FieldLineEdit *newEdit = addFieldLineEdit();
    newEdit->setCompletionItems(d->completionItems);
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
    newEdit->setFocus(Qt::ShortcutFocusReason);
    emit modified();
}

void FieldListEdit::lineRemove(QWidget *widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit *>(widget);
    d->removeFieldLineEdit(fieldLineEdit);
    QSize size(d->container->width(), d->recommendedHeight());
    d->container->resize(size);
}

void FieldListEdit::lineGoDown(QWidget *widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit *>(widget);
    d->goDownFieldLineEdit(fieldLineEdit);
}

void FieldListEdit::lineGoUp(QWidget *widget)
{
    FieldLineEdit *fieldLineEdit = static_cast<FieldLineEdit *>(widget);
    d->goUpFieldLineEdit(fieldLineEdit);

}

PersonListEdit::PersonListEdit(KBibTeX::TypeFlag preferredTypeFlag, KBibTeX::TypeFlags typeFlags, QWidget *parent)
        : FieldListEdit(preferredTypeFlag, typeFlags, parent)
{
    m_checkBoxOthers = new QCheckBox(i18n("... and others (et al.)"), this);
    connect(m_checkBoxOthers, SIGNAL(toggled(bool)), this, SIGNAL(modified()));
    QBoxLayout *boxLayout = static_cast<QBoxLayout *>(layout());
    boxLayout->addWidget(m_checkBoxOthers);

    m_buttonAddNamesFromClipboard = new KPushButton(KIcon("edit-paste"), i18n("Add from Clipboard"), this);
    m_buttonAddNamesFromClipboard->setToolTip(i18n("Add a list of names from clipboard"));
    addButton(m_buttonAddNamesFromClipboard);
    connect(m_buttonAddNamesFromClipboard, SIGNAL(clicked()), this, SLOT(slotAddNamesFromClipboard()));
}

bool PersonListEdit::reset(const Value &value)
{
    Value internal = value;

    m_checkBoxOthers->setCheckState(Qt::Unchecked);
    QSharedPointer<PlainText> pt;
    if (!internal.isEmpty() && !(pt = internal.last().dynamicCast<PlainText>()).isNull()) {
        if (pt->text() == QLatin1String("others")) {
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
        value.append(QSharedPointer<PlainText>(new PlainText(QLatin1String("others"))));

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
        QList<QSharedPointer<Person> > personList = FileImporterBibTeX::splitNames(text);
        foreach(QSharedPointer<Person> person, personList) {
            Value *value = new Value();
            value->append(person);
            lineAdd(value);
        }
        if (!personList.isEmpty())
            emit modified();
    }
}


UrlListEdit::UrlListEdit(QWidget *parent)
        : FieldListEdit(KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, parent)
{
    m_signalMapperSaveLocallyButtonClicked = new QSignalMapper(this);
    connect(m_signalMapperSaveLocallyButtonClicked, SIGNAL(mapped(QWidget *)), this, SLOT(slotSaveLocally(QWidget *)));
    m_signalMapperFieldLineEditTextChanged = new QSignalMapper(this);
    connect(m_signalMapperFieldLineEditTextChanged, SIGNAL(mapped(QWidget *)), this, SLOT(textChanged(QWidget *)));

    /// Button to add a reference (i.e. only the filename or URL) to an entry
    m_addReferenceToFile = new KPushButton(KIcon("emblem-symbolic-link"), i18n("Add reference to file ..."), this);
    addButton(m_addReferenceToFile);
    connect(m_addReferenceToFile, SIGNAL(clicked()), this, SLOT(slotAddReferenceToFile()));

    /// Button to copy a file near the BibTeX file (e.g. same folder) and then
    /// add the copy's relative filename to the entry
    m_copyFile = new KPushButton(KIcon("document-save-all"), i18n("Insert file ..."), this);
    addButton(m_copyFile);
    connect(m_copyFile, SIGNAL(clicked()), this, SLOT(slotCopyFile()));
}

UrlListEdit::~UrlListEdit()
{
    delete m_signalMapperSaveLocallyButtonClicked;
    delete m_signalMapperFieldLineEditTextChanged;
}

void UrlListEdit::slotAddReferenceToFile()
{
    QUrl bibtexUrl(d->file != NULL ? d->file->property(File::Url, QVariant()).toUrl() : QUrl());
    QFileInfo bibtexUrlInfo = bibtexUrl.isEmpty() ? QFileInfo() : QFileInfo(bibtexUrl.path());

    const QString filename = KFileDialog::getOpenFileName(KUrl(bibtexUrlInfo.absolutePath()), QString(), this, i18n("Select File"));
    if (!filename.isEmpty()) {
        const QString visibleFilename = askRelativeOrStaticFilename(this, filename, bibtexUrl);
        Value *value = new Value();
        value->append(QSharedPointer<VerbatimText>(new VerbatimText(visibleFilename)));
        lineAdd(value);
        emit modified();
    }
}

void UrlListEdit::slotCopyFile()
{
    QUrl bibtexUrl(d->file != NULL ? d->file->property(File::Url, QVariant()).toUrl() : QUrl());
    if (!bibtexUrl.isValid() || bibtexUrl.isEmpty()) {
        KMessageBox::information(this, i18n("The current BibTeX document has to be saved first before associating other files with this document."));
        return;
    }
    /// Gather information on BibTeX file's URL
    QFileInfo bibtexUrlInfo = QFileInfo(bibtexUrl.path());

    /// Ask user which file to associate with the BibTeX document
    // TODO allow a similar operation by drag-and-drop of e.g. PDF files
    const QString sourceFilename = KFileDialog::getOpenFileName(KUrl(), QString(), this, i18n("Select Source File"));
    if (!sourceFilename.isEmpty()) {
        /// Build a proposal for the new filename relative to the BibTeX document
        // TODO make this more configurable, e.g. via templates
        const Entry *entry = dynamic_cast<const Entry *>(m_element);
        const QString entryKey = entry != NULL ? entry->id() : QLatin1String("");
        int p = sourceFilename.lastIndexOf(QLatin1Char('.'));
        const QString extension = p > 0 && !entryKey.isEmpty() ? sourceFilename.mid(p) : QString();
        QString destinationFilename = bibtexUrlInfo.absolutePath() + QDir::separator() + entryKey + extension;

        /// Ask user for confirmation on the new filename
        destinationFilename = KFileDialog::getSaveFileName(KUrl(destinationFilename), QString(), this, i18n("Select Destination"));
        if (!destinationFilename.isEmpty()) {
            /// Memorize the absolute filename for the copy operation further below
            const QString absoluteDestinationFilename = destinationFilename;

            /// Determine the relative filename (relative to the BibTeX file)
            QFileInfo destinationFilenameInfo(destinationFilename);
            if (destinationFilenameInfo.absolutePath() == bibtexUrlInfo.absolutePath() || destinationFilenameInfo.absolutePath().startsWith(bibtexUrlInfo.absolutePath() + QDir::separator())) {
                QString relativePath = destinationFilenameInfo.absolutePath().mid(bibtexUrlInfo.absolutePath().length() + 1);
                destinationFilename = relativePath + (relativePath.isEmpty() ? QLatin1String("") : QString(QDir::separator())) + destinationFilenameInfo.fileName();
            }

            /// If file with same destination filename exists, delete it
            KIO::NetAccess::del(KUrl(absoluteDestinationFilename), this);
            /// Copy associated document to destination using the absolute filename
            if (KIO::NetAccess::file_copy(KUrl(sourceFilename), KUrl(absoluteDestinationFilename), this)) {
                /// Only if copy operation succeeded, add reference to copied file into BibTeX entry
                Value *value = new Value();
                value->append(QSharedPointer<VerbatimText>(new VerbatimText(destinationFilename)));
                lineAdd(value);
                emit modified();
            }
        }
    }
}

void UrlListEdit::slotSaveLocally(QWidget *widget)
{
    /// Determine FieldLineEdit widget
    FieldLineEdit *fieldLineEdit = dynamic_cast<FieldLineEdit *>(widget);
    /// Build Url from line edit's content
    const KUrl url(fieldLineEdit->text());

    /// Only proceed if Url is valid and points to a remote location
    if (url.isValid() && !urlIsLocal(url)) {
        /// Get filename from url (without any path/directory part)
        QString filename = url.fileName();
        /// Build QFileInfo from current BibTeX file if available
        QFileInfo bibFileinfo = d->file != NULL ? QFileInfo(d->file->property(File::Url).toUrl().path()) : QFileInfo();
        /// Build proposal to a local filename for remote file
        filename = bibFileinfo.isFile() ? bibFileinfo.absolutePath() + QDir::separator() + filename : filename;
        /// Ask user for actual local filename to save remote file to
        filename = KFileDialog::getSaveFileName(filename, QLatin1String("application/pdf application/postscript image/vnd.djvu"), this, i18n("Save file locally"));
        /// Check if user entered a valid filename ...
        if (!filename.isEmpty()) {
            /// Ask user if reference to local file should be
            /// relative or absolute in relation to the BibTeX file
            const QString absoluteFilename = filename;
            QString visibleFilename = filename;
            if (bibFileinfo.isFile())
                visibleFilename = askRelativeOrStaticFilename(this, absoluteFilename, d->file->property(File::Url).toUrl());

            /// Download remote file and save it locally
            // FIXME: KIO::NetAccess::download is blocking
            setEnabled(false);
            setCursor(Qt::WaitCursor);
            if (KIO::NetAccess::file_copy(url, KUrl::fromLocalFile(absoluteFilename), this)) {
                /// Download succeeded, add reference to local file to this BibTeX entry
                Value *value = new Value();
                value->append(QSharedPointer<VerbatimText>(new VerbatimText(visibleFilename)));
                lineAdd(value);
                emit modified();
            }
            setEnabled(true);
            unsetCursor();
        }
    }
}

void UrlListEdit::textChanged(QWidget *widget)
{
    /// Determine associated KPushButton "Save locally"
    KPushButton *buttonSaveLocally = dynamic_cast<KPushButton *>(widget);
    if (buttonSaveLocally == NULL) return; ///< should never happen!

    /// Assume a FieldLineEdit was the sender of this signal
    FieldLineEdit *fieldLineEdit = dynamic_cast<FieldLineEdit *>(m_signalMapperFieldLineEditTextChanged->mapping(widget));
    if (fieldLineEdit == NULL) return; ///< should never happen!

    /// Create URL from new text to make some tests on it
    /// Only remote URLs are of interest, therefore no tests
    /// on local file or relative paths
    QString newText = fieldLineEdit->text();
    KUrl url(newText);
    newText = newText.toLower();

    /// Enable button only if Url is valid and points to a remote
    /// DjVu, PDF, or PostScript file
    // TODO more file types?
    bool canBeSaved = url.isValid() && !urlIsLocal(url) && (newText.endsWith(QLatin1String(".djvu")) || newText.endsWith(QLatin1String(".pdf")) || newText.endsWith(QLatin1String(".ps"))) && !urlIsLocal(url);
    buttonSaveLocally->setEnabled(canBeSaved);
    buttonSaveLocally->setToolTip(canBeSaved ? i18n("Save file '%1' locally", url.pathOrUrl()) : QLatin1String(""));
}

QString UrlListEdit::askRelativeOrStaticFilename(QWidget *parent, const QString &absoluteFilename, const QUrl &baseUrl)
{
    QFileInfo baseUrlInfo = baseUrl.isEmpty() ? QFileInfo() : QFileInfo(baseUrl.path());
    QFileInfo filenameInfo(absoluteFilename);
    if (!baseUrl.isEmpty() && (filenameInfo.absolutePath() == baseUrlInfo.absolutePath() || filenameInfo.absolutePath().startsWith(baseUrlInfo.absolutePath() + QDir::separator()))) {
        // TODO cover level-up cases like "../../test.pdf"
        const QString relativePath = filenameInfo.absolutePath().mid(baseUrlInfo.absolutePath().length() + 1);
        const QString relativeFilename = relativePath + (relativePath.isEmpty() ? QLatin1String("") : QString(QDir::separator())) + filenameInfo.fileName();
        if (KMessageBox::questionYesNo(parent, i18n("<qt><p>Use a filename relative to the bibliography file?</p><p>The relative path would be<br/><tt style=\"font-family: %3;\">%1</tt></p><p>The absolute path would be<br/><tt style=\"font-family: %3;\">%2</tt></p></qt>", relativeFilename, absoluteFilename, KGlobalSettings::fixedFont().family()), i18n("Relative Path"), KGuiItem(i18n("Relative Path")), KGuiItem(i18n("Absolute Path"))) == KMessageBox::Yes)
            return relativeFilename;
    }
    return absoluteFilename;
}

bool UrlListEdit::urlIsLocal(const QUrl &url)
{
    const QString scheme = url.scheme();
    /// Test various schemes such as "http", "https", "ftp", ...
    return !scheme.startsWith(QLatin1String("http")) && !scheme.startsWith(QLatin1String("ftp")) && !scheme.startsWith(QLatin1String("webdav")) && scheme != QLatin1String("smb");
}

FieldLineEdit *UrlListEdit::addFieldLineEdit()
{
    /// Call original implementation to get an instance of a FieldLineEdit
    FieldLineEdit *fieldLineEdit = FieldListEdit::addFieldLineEdit();

    /// Create a new "save locally" button
    KPushButton *buttonSaveLocally = new KPushButton(KIcon("document-save"), QLatin1String(""), fieldLineEdit);
    buttonSaveLocally->setToolTip(i18n("Save file locally"));
    buttonSaveLocally->setEnabled(false);
    /// Append button to new FieldLineEdit
    fieldLineEdit->appendWidget(buttonSaveLocally);
    /// Connect signals to react on button events
    /// or changes in the FieldLineEdit's text
    m_signalMapperSaveLocallyButtonClicked->setMapping(buttonSaveLocally, fieldLineEdit);
    m_signalMapperFieldLineEditTextChanged->setMapping(fieldLineEdit, buttonSaveLocally);
    connect(buttonSaveLocally, SIGNAL(clicked()), m_signalMapperSaveLocallyButtonClicked, SLOT(map()));
    connect(fieldLineEdit, SIGNAL(textChanged(QString)), m_signalMapperFieldLineEditTextChanged, SLOT(map()));

    return fieldLineEdit;
}

void UrlListEdit::setReadOnly(bool isReadOnly)
{
    FieldListEdit::setReadOnly(isReadOnly);
    m_addReferenceToFile->setEnabled(!isReadOnly);
    m_copyFile->setEnabled(!isReadOnly);
}


const QString KeywordListEdit::keyGlobalKeywordList = QLatin1String("globalKeywordList");

KeywordListEdit::KeywordListEdit(QWidget *parent)
        : FieldListEdit(KBibTeX::tfKeyword, KBibTeX::tfKeyword | KBibTeX::tfSource, parent), m_config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), m_configGroupName(QLatin1String("Global Keywords"))
{
    m_buttonAddKeywordsFromList = new KPushButton(KIcon("list-add"), i18n("Add Keywords from List"), this);
    m_buttonAddKeywordsFromList->setToolTip(i18n("Add keywords as selected from a pre-defined list of keywords"));
    addButton(m_buttonAddKeywordsFromList);
    connect(m_buttonAddKeywordsFromList, SIGNAL(clicked()), this, SLOT(slotAddKeywordsFromList()));
    m_buttonAddKeywordsFromClipboard = new KPushButton(KIcon("edit-paste"), i18n("Add Keywords from Clipboard"), this);
    m_buttonAddKeywordsFromClipboard->setToolTip(i18n("Add a punctuation-separated list of keywords from clipboard"));
    addButton(m_buttonAddKeywordsFromClipboard);
    connect(m_buttonAddKeywordsFromClipboard, SIGNAL(clicked()), this, SLOT(slotAddKeywordsFromClipboard()));
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
    foreach(const QString &keyword, keywords)
    forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// insert all unique keywords used in this file
    foreach(const QString &keyword, m_keywordsFromFile)
    forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// re-create string list from map's values
    keywords = forCaseInsensitiveSorting.values();

    bool ok = false;
    const QStringList newKeywordList = KInputDialog::getItemList(i18n("Add Keywords"), i18n("Select keywords to add:"), keywords, QStringList(), true, &ok, this);
    if (ok) {
        foreach(const QString &newKeywordText, newKeywordList) {
            Value *value = new Value();
            value->append(QSharedPointer<Keyword>(new Keyword(newKeywordText)));
            lineAdd(value);
        }
        if (!newKeywordList.isEmpty())
            emit modified();
    }
}

void KeywordListEdit::slotAddKeywordsFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString text = clipboard->text(QClipboard::Clipboard);
    if (text.isEmpty())
        text = clipboard->text(QClipboard::Selection);
    if (!text.isEmpty()) {
        QList<QSharedPointer<Keyword> > keywords = FileImporterBibTeX::splitKeywords(text);
        for (QList<QSharedPointer<Keyword> >::ConstIterator it = keywords.constBegin(); it != keywords.constEnd(); ++it) {
            Value *value = new Value();
            value->append(*it);
            lineAdd(value);
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
    if (file == NULL)
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
    foreach(const QString &keyword, keywords)
    forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// insert all unique keywords used in this file
    foreach(const QString &keyword, items)
    forCaseInsensitiveSorting.insert(keyword.toLower(), keyword);
    /// re-create string list from map's values
    keywords = forCaseInsensitiveSorting.values();

    FieldListEdit::setCompletionItems(keywords);
}
