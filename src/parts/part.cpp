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

#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QLayout>
#include <QKeyEvent>
#include <QSignalMapper>

#include <KFileDialog>
#include <KDebug>
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
#include <KIO/NetAccess>
#include <KRun>
#include <KMimeType>

#include "file.h"
#include "macro.h"
#include "preamble.h"
#include "comment.h"
#include "fileinfo.h"
#include "fileexporterbibtexoutput.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "fileimporterris.h"
#include "fileexporterris.h"
#include "fileimporterpdf.h"
#include "fileexporterps.h"
#include "fileexporterpdf.h"
#include "fileexporterrtf.h"
#include "fileexporterbibtex2html.h"
#include "fileexporterxml.h"
#include "fileexporterxslt.h"
#include "bibtexfilemodel.h"
#include "filesettingswidget.h"
#include "filterbar.h"
#include "findduplicatesui.h"
#include "lyx.h"
#include "preferences.h"
#include "settingscolorlabelwidget.h"
#include "settingsfileexporterpdfpswidget.h"
#include "findpdfui.h"
#include "valuelistmodel.h"
#include "clipboard.h"
#include "idsuggestions.h"
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
    KSharedConfigPtr config;

public:
    File *bibTeXFile;
    BibTeXEditor *editor;
    BibTeXFileModel *model;
    SortFilterBibTeXFileModel *sortFilterProxyModel;
    FilterBar *filterBar;
    QSignalMapper *signalMapperNewElement;
    KAction *editCutAction, *editDeleteAction, *editCopyAction, *editPasteAction, *editCopyReferencesAction, *elementEditAction, *elementViewDocumentAction, *fileSaveAction, *elementFindPDFAction, *entryApplyDefaultFormatString;
    KMenu *viewDocumentMenu;
    QSignalMapper *signalMapperViewDocument;
    bool isSaveAsOperation;
    LyX *lyx;
    FindDuplicatesUI *findDuplicatesUI;
    ColorLabelContextMenu *colorLabelContextMenu;
    KAction *colorLabelContextMenuAction;

    KBibTeXPartPrivate(KBibTeXPart *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), bibTeXFile(NULL), model(NULL), sortFilterProxyModel(NULL), signalMapperNewElement(new QSignalMapper(parent)), viewDocumentMenu(new KMenu(i18n("View Document"), parent->widget())), signalMapperViewDocument(new QSignalMapper(parent)), isSaveAsOperation(false) {
        connect(signalMapperViewDocument, SIGNAL(mapped(QObject *)), p, SLOT(elementViewDocumentMenu(QObject *)));
    }

    ~KBibTeXPartPrivate() {
        delete bibTeXFile;
        delete model;
        delete signalMapperNewElement;
        delete viewDocumentMenu;
        delete signalMapperViewDocument;
    }

    FileImporter *fileImporterFactory(const KUrl &url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "pdf") {
            return new FileImporterPDF();
        } else if (ending == "ris") {
            return new FileImporterRIS();
        } else {
            return new FileImporterBibTeX(false);
        }
    }

    FileExporter *fileExporterFactory(const KUrl &url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "html") {
            return new FileExporterXSLT();
        } else if (ending == "xml") {
            return new FileExporterXML();
        } else if (ending == "ris") {
            return new FileExporterRIS();
        } else if (ending == "pdf") {
            return new FileExporterPDF();
        } else if (ending == "ps") {
            return new FileExporterPS();
        } else if (ending == "rtf") {
            return new FileExporterRTF();
        } else if (ending == "html" || ending == "htm") {
            return new FileExporterBibTeX2HTML();
        } else if (ending == "bbl") {
            return new FileExporterBibTeXOutput(FileExporterBibTeXOutput::BibTeXBlockList);
        } else {
            return new FileExporterBibTeX();
        }
    }

    QString findUnusedId() {
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
        bibTeXFile = new File();
        model = new BibTeXFileModel();
        model->setBibTeXFile(bibTeXFile);

        if (sortFilterProxyModel != NULL) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterBibTeXFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        editor->setModel(sortFilterProxyModel);
        connect(filterBar, SIGNAL(filterChanged(SortFilterBibTeXFileModel::FilterQuery)), sortFilterProxyModel, SLOT(updateFilter(SortFilterBibTeXFileModel::FilterQuery)));
    }

    void makeBackup(const KUrl &url) const {
        /// Do not make backup copies if file does not exist yet
        if (!KIO::NetAccess::exists(url, KIO::NetAccess::DestinationSide, p->widget()))
            return;

        /// Fetch settings from configuration
        KConfigGroup configGroup(config, Preferences::groupGeneral);
        const Preferences::BackupScope backupScope = (Preferences::BackupScope)configGroup.readEntry(Preferences::keyBackupScope, (int)Preferences::defaultBackupScope);
        const int numberOfBackups = configGroup.readEntry(Preferences::keyNumberOfBackups, Preferences::defaultNumberOfBackups);

        /// Stop right here if no backup is requested
        if (backupScope == Preferences::NoBackup)
            return;

        /// For non-local files, proceed only if backups to remote storage is allowed
        if (!url.isLocalFile() && backupScope != Preferences::BothLocalAndRemote)
            return;

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

        if (copySucceeded && (numberOfBackups > 0)) {
            /// copy e.g. test.bib into test.bib~
            KUrl b(url);
            b.setFileName(url.fileName() + QLatin1String("~"));
            KIO::NetAccess::del(b, p->widget());
            copySucceeded = KIO::NetAccess::file_copy(url, b, p->widget());
        }

        if (!copySucceeded)
            KMessageBox::error(p->widget(), i18n("<qt>Could not create backup copies of document<br/><b>%1</b>.</qt>", url.pathOrUrl()), i18n("Backup copies"));
    }

    KUrl getSaveFilename(bool mustBeImportable = true) {
        QString startDir = p->url().isValid() ? p->url().path() : QLatin1String("kfiledialog:///opensave");
        QString supportedMimeTypes = QLatin1String("text/x-bibtex text/x-bibtex-compiled application/xml text/x-research-info-systems");
        if (!mustBeImportable && FileExporterToolchain::which(QLatin1String("pdflatex")))
            supportedMimeTypes += QLatin1String(" application/pdf");
        if (!mustBeImportable && FileExporterToolchain::which(QLatin1String("dvips")))
            supportedMimeTypes += QLatin1String(" application/postscript");
        if (!mustBeImportable)
            supportedMimeTypes += QLatin1String(" text/html");
        if (!mustBeImportable && FileExporterToolchain::which(QLatin1String("latex2rtf")))
            supportedMimeTypes += QLatin1String(" application/rtf");

        KFileDialog saveDlg(startDir, supportedMimeTypes, p->widget());
        /// Setting list of mime types for the second time,
        /// essentially calling this function only to set the "default mime type" parameter
        saveDlg.setMimeFilter(supportedMimeTypes.split(QChar(' '), QString::SkipEmptyParts), QLatin1String("text/x-bibtex"));
        /// Setting the dialog into "Saving" mode make the "add extension" checkbox available
        saveDlg.setOperationMode(KFileDialog::Saving);
        if (saveDlg.exec() != QDialog::Accepted)
            /// User cancelled saving operation, return invalid filename/URL
            return KUrl();
        return saveDlg.selectedUrl();
    }

    bool saveFile(const KUrl &url) {
        Q_ASSERT_X(!url.isEmpty(), "bool KBibTeXPart::KBibTeXPartPrivate:saveFile(const KUrl &url)", "url is not allowed to be empty");

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
        Q_ASSERT_X(model != NULL, "bool KBibTeXPart::KBibTeXPartPrivate:saveFile(const KUrl &url)", "SortFilterBibTeXFileModel *model from editor->model() is invalid");
        FileExporter *exporter = fileExporterFactory(url);

        if (isSaveAsOperation) {
            /// only show export dialog at SaveAs or SaveCopyAs operations
            FileExporterToolchain *fet = NULL;

            if (typeid(*exporter) == typeid(FileExporterBibTeX)) {
                KDialog dlg(p->widget());
                FileSettingsWidget settingsWidget(&dlg);
                settingsWidget.loadProperties(bibTeXFile);
                dlg.setMainWidget(&settingsWidget);
                dlg.setCaption(i18n("BibTeX File Settings"));
                dlg.setButtons(KDialog::Default | KDialog::Reset | KDialog::User1 | KDialog::Ok);
                dlg.setButtonGuiItem(KDialog::User1, KGuiItem(i18n("Save as Default"), KIcon("edit-redo") /** matches reset button's icon */, i18n("Save this configuration as default for future Save As operations.")));
                connect(&dlg, SIGNAL(user1Clicked()), &settingsWidget, SLOT(saveState()));
                connect(&dlg, SIGNAL(resetClicked()), &settingsWidget, SLOT(loadState()));
                connect(&dlg, SIGNAL(defaultClicked()), &settingsWidget, SLOT(resetToDefaults()));
                dlg.exec();
                settingsWidget.saveProperties(bibTeXFile);
            } else if ((fet = dynamic_cast<FileExporterToolchain *>(exporter)) != NULL) {
                KDialog dlg(p->widget());
                SettingsFileExporterPDFPSWidget settingsWidget(&dlg);
                dlg.setMainWidget(&settingsWidget);
                dlg.setCaption(i18n("PDF/PostScript File Settings"));
                dlg.setButtons(KDialog::Default | KDialog::Reset | KDialog::User1 | KDialog::Ok);
                dlg.setButtonGuiItem(KDialog::User1, KGuiItem(i18n("Save as Default"), KIcon("edit-redo") /** matches reset button's icon */, i18n("Save this configuration as default for future Save As operations.")));
                connect(&dlg, SIGNAL(user1Clicked()), &settingsWidget, SLOT(saveState()));
                connect(&dlg, SIGNAL(resetClicked()), &settingsWidget, SLOT(loadState()));
                connect(&dlg, SIGNAL(defaultClicked()), &settingsWidget, SLOT(resetToDefaults()));
                dlg.exec();
                settingsWidget.saveState();
                fet->reloadConfig();
            }
        }

        qApp->setOverrideCursor(Qt::WaitCursor);

        QStringList errorLog;
        bool result = exporter->save(&temporaryFile, model->bibTeXSourceModel()->bibTeXFile(), &errorLog);

        if (!result) {
            delete exporter;
            qApp->restoreOverrideCursor();
            KMessageBox::errorList(p->widget(), i18n("Saving the bibliography to file '%1' failed.\n\nThe following output was generated by the export filter:", url.pathOrUrl()), errorLog, i18n("Saving bibliography failed"));
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
            KMessageBox::error(p->widget(), i18n("Saving the bibliography to file '%1' failed.", url.pathOrUrl()), i18n("Saving bibliography failed"));

        delete exporter;
        return result;
    }

    bool checkOverwrite(const KUrl &url, QWidget *parent) {
        if (!url.isLocalFile())
            return true;

        QFileInfo info(url.path());
        if (!info.exists())
            return true;

        return KMessageBox::Cancel != KMessageBox::warningContinueCancel(parent, i18n("A file named '%1' already exists. Are you sure you want to overwrite it?", info.fileName()), i18n("Overwrite File?"), KStandardGuiItem::overwrite(), KStandardGuiItem::cancel(), QString(), KMessageBox::Notify | KMessageBox::Dangerous);
    }

    int updateViewDocumentMenu() {
        viewDocumentMenu->clear();
        int result = 0;

        QSharedPointer<const Entry> entry = editor->currentElement().dynamicCast<const Entry>();
        if (!entry.isNull()) {
            QList<KUrl> urlList = FileInfo::entryUrls(entry.data(), editor->bibTeXModel()->bibTeXFile()->property(File::Url).toUrl(), FileInfo::TestExistanceYes);
            if (!urlList.isEmpty()) {
                /// First iteration: local references only
                KAction *firstAction = NULL;
                for (QList<KUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
                    if (!(*it).isLocalFile()) continue;

                    // FIXME: the signal mapper will fill up with mappings, as they are never removed
                    QFileInfo fi((*it).pathOrUrl());
                    const QString label = QString("%1 [%2]").arg(fi.fileName()).arg(fi.absolutePath());
                    KAction *action = new KAction(KIcon(KMimeType::iconNameForUrl(*it)), label, p);
                    action->setData((*it).pathOrUrl());
                    action->setToolTip((*it).prettyUrl());
                    connect(action, SIGNAL(triggered()), signalMapperViewDocument, SLOT(map()));
                    signalMapperViewDocument->setMapping(action, action);
                    viewDocumentMenu->addAction(action);
                    if (firstAction == NULL) firstAction = action;
                }
                if (firstAction != NULL)
                    viewDocumentMenu->addTitle(i18n("Local Files"), firstAction);

                firstAction = NULL;
                for (QList<KUrl>::ConstIterator it = urlList.constBegin(); it != urlList.constEnd(); ++it) {
                    if ((*it).isLocalFile()) continue;

                    // FIXME: the signal mapper will fill up with mappings, as they are never removed
                    KAction *action = new KAction(KIcon(KMimeType::iconNameForUrl(*it)), (*it).pathOrUrl(), p);
                    action->setData((*it).pathOrUrl());
                    action->setToolTip((*it).prettyUrl());
                    connect(action, SIGNAL(triggered()), signalMapperViewDocument, SLOT(map()));
                    signalMapperViewDocument->setMapping(action, action);
                    viewDocumentMenu->addAction(action);
                    if (firstAction == NULL) firstAction = action;
                }
                if (firstAction != NULL)
                    viewDocumentMenu->addTitle(i18n("Remote Files"), firstAction);

                result = urlList.count();
            }
        }

        return result;
    }
};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate(this))
{
    setComponentData(KBibTeXPartFactory::componentData());
    setObjectName("KBibTeXPart::KBibTeXPart");

    // TODO Setup view
    d->editor = new BibTeXEditor(QLatin1String("Main"), parentWidget);
    d->editor->setReadOnly(!isReadWrite());
    setWidget(d->editor);

    connect(d->editor, SIGNAL(elementExecuted(QSharedPointer<Element>)), d->editor, SLOT(editElement(QSharedPointer<Element>)));
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
    delete d->findDuplicatesUI;
    delete d;
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
    d->editor->setFilterBar(d->filterBar);
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
    d->elementViewDocumentAction = new KAction(KIcon("application-pdf"), i18n("View Document"), this);
    d->elementViewDocumentAction->setShortcut(Qt::CTRL + Qt::Key_D);
    actionCollection()->addAction(QLatin1String("element_viewdocument"),  d->elementViewDocumentAction);
    connect(d->elementViewDocumentAction, SIGNAL(triggered()), this, SLOT(elementViewDocument()));

    d->elementFindPDFAction = new KAction(KIcon("application-pdf"), i18n("Find PDF ..."), this);
    actionCollection()->addAction(QLatin1String("element_findpdf"),  d->elementFindPDFAction);
    connect(d->elementFindPDFAction, SIGNAL(triggered()), this, SLOT(elementFindPDF()));

    d->entryApplyDefaultFormatString = new KAction(KIcon("favorites"), i18n("Format entry ids"), this);
    actionCollection()->addAction(QLatin1String("entry_applydefaultformatstring"), d->entryApplyDefaultFormatString);
    connect(d->entryApplyDefaultFormatString, SIGNAL(triggered()), this, SLOT(applyDefaultFormatString()));

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

    /// build context menu for central BibTeX file view
    d->editor->setContextMenuPolicy(Qt::ActionsContextMenu);
    d->editor->addAction(d->elementEditAction);
    d->editor->addAction(d->elementViewDocumentAction);
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    d->editor->addAction(separator);
    d->editor->addAction(d->editCutAction);
    d->editor->addAction(d->editCopyAction);
    d->editor->addAction(d->editCopyReferencesAction);
    d->editor->addAction(d->editPasteAction);
    d->editor->addAction(d->editDeleteAction);
    separator = new QAction(this);
    separator->setSeparator(true);
    d->editor->addAction(separator);

    // TODO

    connect(d->editor, SIGNAL(selectedElementsChanged()), this, SLOT(updateActions()));
    connect(d->editor, SIGNAL(currentElementChanged(QSharedPointer<Element>, const File *)), this, SLOT(updateActions()));

    d->editor->addAction(d->elementFindPDFAction);
    d->editor->addAction(d->entryApplyDefaultFormatString);

    d->colorLabelContextMenu = new ColorLabelContextMenu(d->editor);
    d->colorLabelContextMenuAction = actionCollection()->addAction(QLatin1String("entry_colorlabel"), d->colorLabelContextMenu->menuAction());

    setXMLFile(RCFileName);

    d->findDuplicatesUI = new FindDuplicatesUI(this, d->editor);
    d->lyx = new LyX(this, d->editor);

    updateActions();
    fitActionSettings();
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
        KMessageBox::error(widget(), i18n("The document could not be saved, as it was not possible to write to '%1'.\n\nCheck that you have write access to this file or that enough disk space is available.", url().pathOrUrl()));
        return false;
    }

    return true;
}

bool KBibTeXPart::documentSave()
{
    d->isSaveAsOperation = false;
    if (!isReadWrite())
        return documentSaveCopyAs();
    else if (!url().isValid())
        return documentSaveAs();
    else
        return KParts::ReadWritePart::save();
}

bool KBibTeXPart::documentSaveAs()
{
    d->isSaveAsOperation = true;
    KUrl url = d->getSaveFilename();
    if (!url.isValid() || !d->checkOverwrite(url, widget()))
        return false;

    return KParts::ReadWritePart::saveAs(url);
}

bool KBibTeXPart::documentSaveCopyAs()
{
    d->isSaveAsOperation = true;
    KUrl url = d->getSaveFilename(false);
    if (!url.isValid() || !d->checkOverwrite(url, widget()))
        return false;

    /// difference from KParts::ReadWritePart::saveAs:
    /// current document's URL won't be changed
    return d->saveFile(url);
}

void KBibTeXPart::elementViewDocument()
{
    KUrl url;

    QList<QAction *> actionList = d->viewDocumentMenu->actions();
    /// Go through all actions (i.e. document URLs) for this element
    for (QList<QAction *>::ConstIterator it = actionList.constBegin(); it != actionList.constEnd(); ++it) {
        /// Make URL from action's data ...
        KUrl tmpUrl = KUrl((*it)->data().toString());
        /// ... but skip this action if the URL is invalid
        if (!tmpUrl.isValid()) continue;
        if (tmpUrl.isLocalFile()) {
            /// If action's URL points to local file,
            /// keep it and stop search for document
            url = tmpUrl;
            break;
        } else if (!url.isValid())
            /// First valid URL found, keep it
            /// URL is not local, so it may get overwritten by another URL
            url = tmpUrl;
    }

    /// Open selected URL
    if (url.isValid()) {
        /// Guess mime type for url to open
        KMimeType::Ptr mimeType = FileInfo::mimeTypeForUrl(url);
        const QString mimeTypeName = mimeType->name();
        /// Ask KDE subsystem to open url in viewer matching mime type
        KRun::runUrl(url, mimeTypeName, widget(), false, false);
    }
}

void KBibTeXPart::elementViewDocumentMenu(QObject *obj)
{
    QString text = static_cast<QAction *>(obj)->data().toString(); ///< only a KAction will be passed along

    /// Guess mime type for url to open
    KUrl url(text);
    KMimeType::Ptr mimeType = FileInfo::mimeTypeForUrl(url);
    const QString mimeTypeName = mimeType->name();
    /// Ask KDE subsystem to open url in viewer matching mime type
    KRun::runUrl(url, mimeTypeName, widget(), false, false);
}

void KBibTeXPart::elementFindPDF()
{
    QModelIndexList mil = d->editor->selectionModel()->selectedRows();
    if (mil.count() == 1) {
        QSharedPointer<Entry> entry = d->editor->bibTeXModel()->element(d->editor->sortFilterProxyModel()->mapToSource(*mil.constBegin()).row()).dynamicCast<Entry>();
        if (!entry.isNull())
            FindPDFUI::interactiveFindPDF(*entry, *d->bibTeXFile, widget());
    }
}

void KBibTeXPart::applyDefaultFormatString()
{
    QModelIndexList mil = d->editor->selectionModel()->selectedRows();
    foreach(const QModelIndex &index, mil) {
        QSharedPointer<Entry> entry = d->editor->bibTeXModel()->element(d->editor->sortFilterProxyModel()->mapToSource(index).row()).dynamicCast<Entry>();
        if (!entry.isNull()) {
            static IdSuggestions idSuggestions;
            bool success = idSuggestions.applyDefaultFormatId(*entry.data());
            if (!success) {
                KMessageBox::information(widget(), i18n("Cannot apply default formatting for entry ids: No default format specified."));
                break;
            }
        }
    }

}

void KBibTeXPart::fitActionSettings()
{
    // TODO
}

bool KBibTeXPart::openFile()
{
    setObjectName("KBibTeXPart::KBibTeXPart for " + url().pathOrUrl());

    FileImporter *importer = d->fileImporterFactory(url());
    importer->showImportDialog(widget());

    qApp->setOverrideCursor(Qt::WaitCursor);

    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    if (d->bibTeXFile != NULL) delete d->bibTeXFile;
    d->bibTeXFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    if (d->bibTeXFile == NULL) {
        kWarning() << "Opening file failed:" << url().pathOrUrl();
        qApp->restoreOverrideCursor();
        return false;
    }

    d->bibTeXFile->setProperty(File::Url, QUrl(url()));

    d->model->setBibTeXFile(d->bibTeXFile);
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
    QSharedPointer<Entry> newEntry = QSharedPointer<Entry>(new Entry(QLatin1String("Article"), d->findUnusedId()));
    d->model->insertRow(newEntry, d->model->rowCount());
    d->editor->setSelectedElement(newEntry);
    if (d->editor->editElement(newEntry))
        d->editor->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::newMacroTriggered()
{
    QSharedPointer<Macro> newMacro = QSharedPointer<Macro>(new Macro(d->findUnusedId()));
    d->model->insertRow(newMacro, d->model->rowCount());
    d->editor->setSelectedElement(newMacro);
    if (d->editor->editElement(newMacro))
        d->editor->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::newPreambleTriggered()
{
    QSharedPointer<Preamble> newPreamble = QSharedPointer<Preamble>(new Preamble());
    d->model->insertRow(newPreamble, d->model->rowCount());
    d->editor->setSelectedElement(newPreamble);
    if (d->editor->editElement(newPreamble))
        d->editor->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::newCommentTriggered()
{
    QSharedPointer<Comment> newComment = QSharedPointer<Comment>(new Comment());
    d->model->insertRow(newComment, d->model->rowCount());
    d->editor->setSelectedElement(newComment);
    if (d->editor->editElement(newComment))
        d->editor->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::updateActions()
{
    bool emptySelection = d->editor->selectedElements().isEmpty();
    d->elementEditAction->setEnabled(!emptySelection);
    d->editCopyAction->setEnabled(!emptySelection);
    d->editCopyReferencesAction->setEnabled(!emptySelection);
    d->editCutAction->setEnabled(!emptySelection && isReadWrite());
    d->editPasteAction->setEnabled(isReadWrite());
    d->editDeleteAction->setEnabled(!emptySelection && isReadWrite());
    d->elementFindPDFAction->setEnabled(!emptySelection && isReadWrite());
    d->entryApplyDefaultFormatString->setEnabled(!emptySelection && isReadWrite());
    d->colorLabelContextMenu->menuAction()->setEnabled(!emptySelection && isReadWrite());
    d->colorLabelContextMenuAction->setEnabled(!emptySelection && isReadWrite());

    int numDocumentsToView = d->updateViewDocumentMenu();
    /// enable menu item only if there is at least one document to view
    d->elementViewDocumentAction->setEnabled(!emptySelection && numDocumentsToView > 0);
    /// activate sub-menu only if there are at least two documents to view
    d->elementViewDocumentAction->setMenu(numDocumentsToView > 1 ? d->viewDocumentMenu : NULL);
    d->elementViewDocumentAction->setToolTip(numDocumentsToView == 1 ? d->viewDocumentMenu->actions().first()->text() : QLatin1String(""));

    /// update list of references which can be sent to LyX
    QStringList references;
    if (d->editor->selectionModel() != NULL) {
        QModelIndexList mil = d->editor->selectionModel()->selectedRows();
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
            QSharedPointer<Entry> entry = d->editor->bibTeXModel()->element(d->editor->sortFilterProxyModel()->mapToSource(*it).row()).dynamicCast<Entry>();
            if (!entry.isNull())
                references << entry->id();
        }
    }
    d->lyx->setReferences(references);
}
