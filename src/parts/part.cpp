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

#include <QLabel>
#include <QFile>
#include <QApplication>
#include <QLayout>
#include <QKeyEvent>
#include <QSignalMapper>

#include <KDebug>
#include <KEncodingFileDialog>
#include <KMessageBox>
#include <KLocale>
#include <KAction>
#include <KActionCollection>
#include <KStandardAction>
#include <KActionMenu>
#include <KSelectAction>
#include <KToggleAction>
#include <KMenu>
#include <KTemporaryFile>
#include <kio/netaccess.h>

#include <file.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <fileimporterris.h>
#include <fileexporterris.h>
#include <fileimporterpdf.h>
#include <fileexporterpdf.h>
#include <fileexporterbibtex2html.h>
#include <fileexporterxml.h>
#include <fileexporterxslt.h>
#include <bibtexfilemodel.h>
#include <macro.h>
#include <preamble.h>
#include <comment.h>
#include <filterbar.h>

#include <valuelistmodel.h>
#include <clipboard.h>
#include "part.h"
#include "partfactory.h"
// #include "browserextension.h" // FIXME

static const char RCFileName[] = "kbibtexpartui.rc";
static const int smEntry = 1;
static const int smComment = 2;
static const int smPreamble = 3;
static const int smMacro = 4;

class KBibTeXPart::KBibTeXPartPrivate
{
private:
    KBibTeXPart *p;

public:
    BibTeXEditor *editor;
    BibTeXFileModel *model;
    SortFilterBibTeXFileModel *sortFilterProxyModel;
    FilterBar *filterBar;
    QSignalMapper *signalMapperNewElement;
    KAction *editCutAction, *editDeleteAction, *editCopyAction, *editPasteAction, *editCopyReferencesAction, *elementEditAction, *fileSaveAction;

    KBibTeXPartPrivate(KBibTeXPart *parent)
            : p(parent), sortFilterProxyModel(NULL), signalMapperNewElement(new QSignalMapper(parent)) {
        // nothing
    }

    FileImporter *fileImporterFactory(const KUrl& url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "pdf") {
            kDebug() << "Selecting FileImporterPDF" << endl;
            return new FileImporterPDF();
        } else if (ending == "ris") {
            kDebug() << "Selecting FileImporterRIS" << endl;
            return new FileImporterRIS();
        } else {
            kDebug() << "Selecting FileImporterBibTeX" << endl;
            return new FileImporterBibTeX("latex", false);
        }
    }

    FileExporter *fileExporterFactory(const KUrl& url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "html") {
            kDebug() << "Selecting FileExporterXSLT" << endl;
            return new FileExporterXSLT();
        } else if (ending == "xml") {
            kDebug() << "Selecting FileExporterXML" << endl;
            return new FileExporterXML();
        } else if (ending == "ris") {
            kDebug() << "Selecting FileExporterRIS" << endl;
            return new FileExporterRIS();
        } else if (ending == "pdf") {
            kDebug() << "Selecting FileExporterPDF" << endl;
            return new FileExporterPDF();
        } else if (ending == "html" || ending == "html") {
            kDebug() << "Selecting FileExporterBibTeX2HTML" << endl;
            return new FileExporterBibTeX2HTML();
        } else {
            kDebug() << "Selecting FileExporterBibTeX" << endl;
            return new FileExporterBibTeX();
        }
    }

    QString findUnusedId() {
        File *bibTeXFile = model->bibTeXFile();
        int i = 1;
        while (true) {
            QString result = i18n("New%1", i);
            if (!bibTeXFile->containsKey(result))
                return result;
            ++i;
        }
        return QString();
    }

    void initializeNew() {
        model = new BibTeXFileModel();
        model->setBibTeXFile(new File());

        if (sortFilterProxyModel != NULL) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterBibTeXFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        editor->setModel(sortFilterProxyModel);
        connect(filterBar, SIGNAL(filterChanged(SortFilterBibTeXFileModel::FilterQuery)), sortFilterProxyModel, SLOT(updateFilter(SortFilterBibTeXFileModel::FilterQuery)));
        //connect(editor->valueListWidget(), SIGNAL(filterChanged(SortFilterBibTeXFileModel::FilterQuery)), filterBar, SLOT(setFilter(SortFilterBibTeXFileModel::FilterQuery))); //FIXME
    }

    void makeBackup(const KUrl &url) const {
        // FIXME: this will make backups on remote storage, too.
        //     is this an expected behaviour?

        /// do not make backup copies if file does not exist yet
        if (!KIO::NetAccess::exists(url, KIO::NetAccess::DestinationSide, p->widget()))
            return;

        const int numberOfBackups = 5; // TODO make this a configuration option

        bool copySucceeded = true;
        /// copy e.g. test.bib~ to test.bib~2 and test.bib~3 to test.bib~4 etc.
        for (int i = numberOfBackups - 1; copySucceeded && i >= 1; --i) {
            KUrl a(url);
            a.setFileName(url.fileName() + (i > 1 ? QString("~%1").arg(i) : QLatin1String("~")));
            if (KIO::NetAccess::exists(a, KIO::NetAccess::DestinationSide, p->widget())) {
                KUrl b(url);
                b.setFileName(url.fileName() + QString("~%1").arg(i + 1));
                KIO::NetAccess::del(b, p->widget());
                copySucceeded = KIO::NetAccess::file_copy(a, b, p->widget());
            }
        }

        if (copySucceeded) {
            /// copy e.g. test.bib into test.bib~
            KUrl b(url);
            b.setFileName(url.fileName() + QLatin1String("~"));
            KIO::NetAccess::del(b, p->widget());
            copySucceeded = KIO::NetAccess::file_copy(url, b, p->widget());
        }

        if (!copySucceeded)
            KMessageBox::error(p->widget(), i18n("<qt>Could not create backup copies of document<br/><b>%1</b>.</qt>", url.pathOrUrl()), i18n("Backup copies"));
    }

    KEncodingFileDialog::Result getSaveFilenames() {
        QString startDir = QString();// QLatin1String(":save"); // FIXME: Does not work yet
        QString supportedMimeTypes = QLatin1String("text/x-bibtex application/xml");
        if (FileExporterToolchain::kpsewhich(QLatin1String("embedfile.sty")))
            supportedMimeTypes += QLatin1String(" application/pdf");
        // TODO application/x-research-info-systems application/x-endnote-refer
        supportedMimeTypes += QLatin1String(" text/html");

        return KEncodingFileDialog::getSaveUrlAndEncoding(QString(), startDir, supportedMimeTypes, p->widget());
    }

    bool saveFile(const KUrl &url) {
        Q_ASSERT_X(!url.isEmpty(), "bool KBibTeXPart::KBibTeXPartPrivate:saveFile(const KUrl &url)", "url is not allowed to be empty");

        qApp->setOverrideCursor(Qt::WaitCursor);

        /// configure and open temporary file
        KTemporaryFile temporaryFile;
        const QRegExp suffixRegExp("\\.[^.]{1,4}$");
        if (suffixRegExp.indexIn(url.pathOrUrl()) >= 0)
            temporaryFile.setSuffix(suffixRegExp.cap(0));
        temporaryFile.setAutoRemove(true);
        if (!temporaryFile.open())
            return false;

        /// export bibliography data into temporary file
        SortFilterBibTeXFileModel *model = dynamic_cast<SortFilterBibTeXFileModel *>(editor->model());
        Q_ASSERT(model != NULL);
        FileExporter *exporter = fileExporterFactory(url);
        QStringList errorLog;
        bool result = exporter->save(&temporaryFile, model->bibTeXSourceModel()->bibTeXFile(), &errorLog);
        delete exporter;

        if (!result) {
            qApp->restoreOverrideCursor();
            KMessageBox::errorList(p->widget(), i18n("Saving the bibliography to file \"%1\" failed.\n\nThe following output was generated by the export filter:", url.pathOrUrl()), errorLog, i18n("Saving bibliography failed"));
            return false;
        }

        /// close/flush temporary file
        temporaryFile.close();
        /// make backup before overwriting target destination
        makeBackup(url);
        /// upload temporary file to target destination
        KIO::NetAccess::del(url, p->widget());
        result &= KIO::NetAccess::file_copy(temporaryFile.fileName(), url, p->widget());

        qApp->restoreOverrideCursor();
        if (!result)
            KMessageBox::error(p->widget(), i18n("Saving the bibliography to file \"%1\" failed.", url.pathOrUrl()), i18n("Saving bibliography failed"));
        else
            model->bibTeXSourceModel()->bibTeXFile()->setUrl(url); /// store new URL in BibTeX File object

        return result;
    }


    bool checkOverwrite(const KUrl &url, QWidget *parent) {
        if (!url.isLocalFile())
            return true;

        QFileInfo info(url.path());
        if (!info.exists())
            return true;

        return KMessageBox::Cancel != KMessageBox::warningContinueCancel(parent,     i18n("A file named \"%1\" already exists. Are you sure you want to overwrite it?",  info.fileName()),
                i18n("Overwrite File?"), KStandardGuiItem::overwrite(),    KStandardGuiItem::cancel(), QString(), KMessageBox::Notify | KMessageBox::Dangerous);
    }
};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate(this))
{
    setComponentData(KBibTeXPartFactory::componentData());
    setObjectName("KBibTeXPart::KBibTeXPart");

    // TODO Setup view
    d->editor = new BibTeXEditor(parentWidget);
    d->editor->setReadOnly(!isReadWrite());
    setWidget(d->editor);

    connect(d->editor, SIGNAL(elementExecuted(Element*)), d->editor, SLOT(editElement(Element*)));
    connect(d->editor, SIGNAL(modified()), this, SLOT(setModified()));

    setupActions(browserViewWanted);

    /* // FIXME
    if (browserViewWanted)
        new KBibTeXBrowserExtension(this);
        */

    d->initializeNew();

    setModified(false);
}

KBibTeXPart::~KBibTeXPart()
{
    // nothing
}

void KBibTeXPart::setModified(bool modified)
{
    KParts::ReadWritePart::setModified(modified);

    d->fileSaveAction->setEnabled(modified);
}

void KBibTeXPart::setupActions(bool /*browserViewWanted FIXME*/)
{
    d->fileSaveAction = actionCollection()->addAction(KStandardAction::Save, this, SLOT(documentSave()));
    d->fileSaveAction->setEnabled(false);
    actionCollection()->addAction(KStandardAction::SaveAs, this, SLOT(documentSaveAs()));
    KAction *saveCopyAsAction = new KAction(KIcon("document-save"), i18n("Save Copy As..."), this);
    actionCollection()->addAction("file_save_copy_as", saveCopyAsAction);
    connect(saveCopyAsAction, SIGNAL(triggered()), this, SLOT(documentSaveCopyAs()));

    d->filterBar = new FilterBar(0);
    KAction *filterWidgetAction = new KAction(i18n("Filter"), this);
    actionCollection()->addAction("toolbar_filter_widget", filterWidgetAction);
    filterWidgetAction->setShortcut(Qt::CTRL + Qt::Key_F);
    filterWidgetAction->setDefaultWidget(d->filterBar);
    connect(filterWidgetAction, SIGNAL(triggered()), d->filterBar, SLOT(setFocus()));

    KActionMenu *newElementAction = new KActionMenu(KIcon("address-book-new"), i18n("New element"), this);
    actionCollection()->addAction("element_new", newElementAction);
    KMenu *newElementMenu = new KMenu(newElementAction->text(), widget());
    newElementAction->setMenu(newElementMenu);
    connect(newElementAction, SIGNAL(triggered()), this, SLOT(newEntryTriggered()));
    QAction *newEntry = newElementMenu->addAction(KIcon("address-book-new"), i18n("New entry"));
    newEntry->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_N);
    connect(newEntry, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newEntry, smEntry);
    QAction *newComment = newElementMenu->addAction(KIcon("address-book-new"), i18n("New comment"));
    connect(newComment, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newComment, smComment);
    QAction *newMacro = newElementMenu->addAction(KIcon("address-book-new"), i18n("New macro"));
    connect(newMacro, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newMacro, smMacro);
    QAction *newPreamble = newElementMenu->addAction(KIcon("address-book-new"), i18n("New preamble"));
    connect(newPreamble, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newPreamble, smPreamble);
    connect(d->signalMapperNewElement, SIGNAL(mapped(int)), this, SLOT(newElementTriggered(int)));
    d->elementEditAction = new KAction(KIcon("document-edit"), i18n("Edit Element"), this);
    d->elementEditAction->setShortcut(Qt::CTRL + Qt::Key_E);
    actionCollection()->addAction(QLatin1String("element_edit"),  d->elementEditAction);
    connect(d->elementEditAction, SIGNAL(triggered()), d->editor, SLOT(editCurrentElement()));


    Clipboard *clipboard = new Clipboard(d->editor);

    d->editCopyReferencesAction = new KAction(KIcon("edit-copy"), i18n("Copy References"), this);
    d->editCopyReferencesAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C);
    actionCollection()->addAction(QLatin1String("edit_copy_references"),  d->editCopyReferencesAction);
    connect(d->editCopyReferencesAction, SIGNAL(triggered()), clipboard, SLOT(copyReferences()));

    d->editDeleteAction = new KAction(KIcon("edit-table-delete-row"), i18n("Delete"), this);
    d->editDeleteAction->setShortcut(Qt::Key_Delete);
    actionCollection()->addAction(QLatin1String("edit_delete"),  d->editDeleteAction);
    connect(d->editDeleteAction, SIGNAL(triggered()), d->editor, SLOT(selectionDelete()));

    d->editCutAction = actionCollection()->addAction(KStandardAction::Cut, clipboard, SLOT(cut()));
    d->editCopyAction = actionCollection()->addAction(KStandardAction::Copy, clipboard, SLOT(copy()));
    actionCollection()->addAction(QLatin1String("edit_copy_references"),  d->editCopyReferencesAction);
    d->editPasteAction = actionCollection()->addAction(KStandardAction::Paste, clipboard, SLOT(paste()));

    d->editor->setContextMenuPolicy(Qt::ActionsContextMenu);
    d->editor->insertAction(NULL, d->elementEditAction);
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    d->editor->insertAction(NULL, separator);
    d->editor->insertAction(NULL, d->editCutAction);
    d->editor->insertAction(NULL, d->editCopyAction);
    d->editor->insertAction(NULL, d->editCopyReferencesAction);
    d->editor->insertAction(NULL, d->editPasteAction);
    d->editor->insertAction(NULL, d->editDeleteAction);

    // TODO

    connect(d->editor, SIGNAL(selectedElementsChanged()), this, SLOT(updateActions()));
    updateActions();

    fitActionSettings();

    setXMLFile(RCFileName);
}

bool KBibTeXPart::saveFile()
{
    Q_ASSERT_X(isReadWrite(), "bool KBibTeXPart::saveFile()", "Trying to save although document is in read-only mode");

    if (url().isEmpty()) {
        kDebug() << "unexpected: url is empty";
        documentSaveAs();
        return false;
    }

    if (!d->saveFile(localFilePath())) {
        KMessageBox::error(widget(), i18n("The document could not be saved, as it was not possible to write to %1.\n\nCheck that you have write access to this file or that enough disk space is available.", url().pathOrUrl()));
        return false;
    }

    return true;
}

bool KBibTeXPart::documentSave()
{
    if (!isReadWrite())
        return documentSaveCopyAs();
    else if (!url().isValid())
        return documentSaveAs();
    else
        return KParts::ReadWritePart::save();
}

bool KBibTeXPart::documentSaveAs()
{
    KEncodingFileDialog::Result res = d->getSaveFilenames();
    if (res.URLs.isEmpty() || !d->checkOverwrite(res.URLs.first(), widget()))
        return false;

    return KParts::ReadWritePart::saveAs(res.URLs.first());
}

bool KBibTeXPart::documentSaveCopyAs()
{
    KEncodingFileDialog::Result res = d->getSaveFilenames();
    if (res.URLs.isEmpty() || !d->checkOverwrite(res.URLs.first(), widget()))
        return false;

    /// difference from KParts::ReadWritePart::saveAs:
    /// current document's URL won't be changed
    return d->saveFile(res.URLs.first());
}

void KBibTeXPart::fitActionSettings()
{
    // TODO
}

bool KBibTeXPart::openFile()
{
    kDebug() << "Opening file: " << url();
    qApp->setOverrideCursor(Qt::WaitCursor);

    setObjectName("KBibTeXPart::KBibTeXPart for " + url().pathOrUrl());

    FileImporter *importer = d->fileImporterFactory(url());
    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    File *bibtexFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    if (bibtexFile == NULL) {
        kWarning() << "Opening file failed: " << url();
        return false;
    } else
        kDebug() << "File contains " << bibtexFile->count() << " entries";

    bibtexFile->setUrl(url());

    d->model->setBibTeXFile(bibtexFile);
    d->editor->setModel(d->model);
    if (d->sortFilterProxyModel != NULL) delete d->sortFilterProxyModel;
    d->sortFilterProxyModel = new SortFilterBibTeXFileModel(this);
    d->sortFilterProxyModel->setSourceModel(d->model);
    d->editor->setModel(d->sortFilterProxyModel);
    connect(d->filterBar, SIGNAL(filterChanged(SortFilterBibTeXFileModel::FilterQuery)), d->sortFilterProxyModel, SLOT(updateFilter(SortFilterBibTeXFileModel::FilterQuery)));

    qApp->restoreOverrideCursor();

    emit completed();
    return true;
}

void KBibTeXPart::newElementTriggered(int event)
{
    switch (event) {
    case smComment:
        newCommentTriggered();
        break;
        break;
    case smMacro:
        newMacroTriggered();
        break;
    case smPreamble:
        newPreambleTriggered();
        break;
    default:
        newEntryTriggered();
    }
}

void KBibTeXPart::newEntryTriggered()
{
    Entry *newEntry = new Entry(QLatin1String("Article"), d->findUnusedId());
    d->model->insertRow(newEntry, d->model->rowCount());
    d->editor->setSelectedElement(newEntry);
    d->editor->editElement(newEntry);
    d->editor->scrollToBottom();
}

void KBibTeXPart::newMacroTriggered()
{
    Macro *newMacro = new Macro(d->findUnusedId());
    d->model->insertRow(newMacro, d->model->rowCount());
    d->editor->setSelectedElement(newMacro);
    d->editor->editElement(newMacro);
    d->editor->scrollToBottom();
}

void KBibTeXPart::newPreambleTriggered()
{
    Preamble *newPreamble = new Preamble();
    d->model->insertRow(newPreamble, d->model->rowCount());
    d->editor->setSelectedElement(newPreamble);
    d->editor->editElement(newPreamble);
    d->editor->scrollToBottom();
}

void KBibTeXPart::newCommentTriggered()
{
    Comment *newComment = new Comment();
    d->model->insertRow(newComment, d->model->rowCount());
    d->editor->setSelectedElement(newComment);
    d->editor->editElement(newComment);
    d->editor->scrollToBottom();
}

void KBibTeXPart::updateActions()
{
    bool emptySelection = d->editor->selectedElements().isEmpty();
    d->elementEditAction->setEnabled(!emptySelection);
    d->editCopyAction->setEnabled(!emptySelection);
    d->editCopyReferencesAction->setEnabled(!emptySelection);
    d->editCutAction->setEnabled(!emptySelection);
    d->editDeleteAction->setEnabled(!emptySelection);
}
