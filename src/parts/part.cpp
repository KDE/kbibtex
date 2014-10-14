/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "part.h"

#include <typeinfo>

#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QApplication>
#include <QLayout>
#include <QKeyEvent>
#include <QSignalMapper>
#include <QtCore/QPointer>
#include <QFileSystemWatcher>

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
#include <KStandardDirs>

#include "file.h"
#include "macro.h"
#include "preamble.h"
#include "comment.h"
#include "fileinfo.h"
#include "fileexporterbibtexoutput.h"
#include "fileimporterbibtex.h"
#include "fileexporterbibtex.h"
#include "fileimporterris.h"
#include "fileimporterbibutils.h"
#include "fileexporterris.h"
#include "fileexporterbibutils.h"
#include "fileimporterpdf.h"
#include "fileexporterps.h"
#include "fileexporterpdf.h"
#include "fileexporterrtf.h"
#include "fileexporterbibtex2html.h"
#include "fileexporterxml.h"
#include "fileexporterxslt.h"
#include "filemodel.h"
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
#include "partfactory.h"
#include "fileview.h"
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
    PartWidget *partWidget;
    FileModel *model;
    SortFilterFileModel *sortFilterProxyModel;
    QSignalMapper *signalMapperNewElement;
    KAction *editCutAction, *editDeleteAction, *editCopyAction, *editPasteAction, *editCopyReferencesAction, *elementEditAction, *elementViewDocumentAction, *fileSaveAction, *elementFindPDFAction, *entryApplyDefaultFormatString;
    KMenu *viewDocumentMenu;
    QSignalMapper *signalMapperViewDocument;
    bool isSaveAsOperation;
    LyX *lyx;
    FindDuplicatesUI *findDuplicatesUI;
    ColorLabelContextMenu *colorLabelContextMenu;
    KAction *colorLabelContextMenuAction;
    QFileSystemWatcher fileSystemWatcher;

    KBibTeXPartPrivate(KBibTeXPart *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), bibTeXFile(NULL), model(NULL), sortFilterProxyModel(NULL), signalMapperNewElement(new QSignalMapper(parent)), viewDocumentMenu(new KMenu(i18n("View Document"), parent->widget())), signalMapperViewDocument(new QSignalMapper(parent)), isSaveAsOperation(false), fileSystemWatcher(p) {
        connect(signalMapperViewDocument, SIGNAL(mapped(QObject*)), p, SLOT(elementViewDocumentMenu(QObject*)));
        connect(&fileSystemWatcher, SIGNAL(fileChanged(QString)), p, SLOT(fileExternallyChange(QString)));
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
        } else if (BibUtils::available() && ending == "isi") {
            FileImporterBibUtils *fileImporterBibUtils = new FileImporterBibUtils();
            fileImporterBibUtils->setFormat(BibUtils::ISI);
            return fileImporterBibUtils;
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
        } else if (BibUtils::available() && ending == "isi") {
            FileExporterBibUtils *fileExporterBibUtils = new FileExporterBibUtils();
            fileExporterBibUtils->setFormat(BibUtils::ISI);
            return fileExporterBibUtils;
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
        model = new FileModel();
        model->setBibliographyFile(bibTeXFile);

        if (sortFilterProxyModel != NULL) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        partWidget->fileView()->setModel(sortFilterProxyModel);
        connect(partWidget->filterBar(), SIGNAL(filterChanged(SortFilterFileModel::FilterQuery)), sortFilterProxyModel, SLOT(updateFilter(SortFilterFileModel::FilterQuery)));
    }

    bool openFile(const KUrl &url, const QString &localFilePath) {
        p->setObjectName("KBibTeXPart::KBibTeXPart for " + url.pathOrUrl());

        FileImporter *importer = fileImporterFactory(url);
        importer->showImportDialog(p->widget());

        qApp->setOverrideCursor(Qt::WaitCursor);

        if (bibTeXFile != NULL) {
            const QUrl oldUrl = bibTeXFile->property(File::Url, QUrl()).toUrl();
            if (oldUrl.isValid() && oldUrl.isLocalFile())
                fileSystemWatcher.removePath(oldUrl.toString());
            else
                kWarning() << "KBibTeXPartPrivate::openFile: Not removing" << oldUrl.toString() << "from fileSystemWatcher";
            delete bibTeXFile;
        }

        QFile inputfile(localFilePath);
        inputfile.open(QIODevice::ReadOnly);
        bibTeXFile = importer->load(&inputfile);
        inputfile.close();
        delete importer;

        if (bibTeXFile == NULL) {
            kWarning() << "Opening file failed:" << url.pathOrUrl();
            qApp->restoreOverrideCursor();
            return false;
        }

        bibTeXFile->setProperty(File::Url, QUrl(url));

        model->setBibliographyFile(bibTeXFile);
        partWidget->fileView()->setModel(model);
        if (sortFilterProxyModel != NULL) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        partWidget->fileView()->setModel(sortFilterProxyModel);
        connect(partWidget->filterBar(), SIGNAL(filterChanged(SortFilterFileModel::FilterQuery)), sortFilterProxyModel, SLOT(updateFilter(SortFilterFileModel::FilterQuery)));

        if (url.isLocalFile()) {
            const QStringList filesBefore = fileSystemWatcher.files();
            const QString toBeAdded = url.pathOrUrl();
            fileSystemWatcher.addPath(toBeAdded);
            const QStringList filesAfter = fileSystemWatcher.files();
            kWarning() << "toBeAdded:" << toBeAdded;
            kWarning() << "files before:" << filesBefore.count() << " files after:" << filesAfter.count();
            kWarning() << "before included?" << filesBefore.contains(toBeAdded) << " after included?" << filesAfter.contains(toBeAdded);
        }

        qApp->restoreOverrideCursor();

        return true;
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
        if (BibUtils::available())
            supportedMimeTypes += QLatin1String(" application/x-isi-export-format application/x-endnote-refer");
        if (!mustBeImportable && !KStandardDirs::findExe(QLatin1String("pdflatex")).isEmpty())
            supportedMimeTypes += QLatin1String(" application/pdf");
        if (!mustBeImportable && !KStandardDirs::findExe(QLatin1String("dvips")).isEmpty())
            supportedMimeTypes += QLatin1String(" application/postscript");
        if (!mustBeImportable)
            supportedMimeTypes += QLatin1String(" text/html");
        if (!mustBeImportable && !KStandardDirs::findExe(QLatin1String("latex2rtf")).isEmpty())
            supportedMimeTypes += QLatin1String(" application/rtf");

        QPointer<KFileDialog> saveDlg = new KFileDialog(startDir, supportedMimeTypes, p->widget());
        /// Setting list of mime types for the second time,
        /// essentially calling this function only to set the "default mime type" parameter
        saveDlg->setMimeFilter(supportedMimeTypes.split(QChar(' '), QString::SkipEmptyParts), QLatin1String("text/x-bibtex"));
        /// Setting the dialog into "Saving" mode make the "add extension" checkbox available
        saveDlg->setOperationMode(KFileDialog::Saving);
        if (saveDlg->exec() != QDialog::Accepted)
            /// User cancelled saving operation, return invalid filename/URL
            return KUrl();
        const KUrl selectedUrl = saveDlg->selectedUrl();
        delete saveDlg;
        return selectedUrl;
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
        SortFilterFileModel *model = qobject_cast<SortFilterFileModel *>(partWidget->fileView()->model());
        Q_ASSERT_X(model != NULL, "bool KBibTeXPart::KBibTeXPartPrivate:saveFile(const KUrl &url)", "SortFilterFileModel *model from editor->model() is invalid");
        FileExporter *exporter = fileExporterFactory(url);

        if (isSaveAsOperation) {
            /// only show export dialog at SaveAs or SaveCopyAs operations
            FileExporterToolchain *fet = NULL;

            if (typeid(*exporter) == typeid(FileExporterBibTeX)) {
                QPointer<KDialog> dlg = new KDialog(p->widget());
                FileSettingsWidget *settingsWidget = new FileSettingsWidget(dlg);
                settingsWidget->loadProperties(bibTeXFile);
                dlg->setMainWidget(settingsWidget);
                dlg->setCaption(i18n("BibTeX File Settings"));
                dlg->setButtons(KDialog::Default | KDialog::Reset | KDialog::User1 | KDialog::Ok);
                dlg->setButtonGuiItem(KDialog::User1, KGuiItem(i18n("Save as Default"), KIcon("edit-redo") /** matches reset button's icon */, i18n("Save this configuration as default for future Save As operations.")));
                connect(dlg, SIGNAL(user1Clicked()), settingsWidget, SLOT(saveProperties()));
                connect(dlg, SIGNAL(resetClicked()), settingsWidget, SLOT(loadProperties()));
                connect(dlg, SIGNAL(defaultClicked()), settingsWidget, SLOT(resetToDefaults()));
                dlg->exec();
                settingsWidget->saveProperties(bibTeXFile);
                delete dlg;
            } else if ((fet = qobject_cast<FileExporterToolchain *>(exporter)) != NULL) {
                QPointer<KDialog> dlg = new KDialog(p->widget());
                SettingsFileExporterPDFPSWidget *settingsWidget = new SettingsFileExporterPDFPSWidget(dlg);
                dlg->setMainWidget(settingsWidget);
                dlg->setCaption(i18n("PDF/PostScript File Settings"));
                dlg->setButtons(KDialog::Default | KDialog::Reset | KDialog::User1 | KDialog::Ok);
                dlg->setButtonGuiItem(KDialog::User1, KGuiItem(i18n("Save as Default"), KIcon("edit-redo") /** matches reset button's icon */, i18n("Save this configuration as default for future Save As operations.")));
                connect(dlg, SIGNAL(user1Clicked()), settingsWidget, SLOT(saveState()));
                connect(dlg, SIGNAL(resetClicked()), settingsWidget, SLOT(loadState()));
                connect(dlg, SIGNAL(defaultClicked()), settingsWidget, SLOT(resetToDefaults()));
                dlg->exec();
                settingsWidget->saveState();
                fet->reloadConfig();
                delete dlg;
            }
        }

        qApp->setOverrideCursor(Qt::WaitCursor);

        QStringList errorLog;
        bool result = exporter->save(&temporaryFile, model->fileSourceModel()->bibliographyFile(), &errorLog);

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
        KUrl realUrl = url;
        if (url.isLocalFile()) {
            /// take precautions for local files
            QFileInfo fileInfo(url.pathOrUrl());
            if (fileInfo.isSymLink()) {
                /// do not overwrite symbolic link,
                /// but linked file instead
                realUrl = KUrl::fromLocalFile(fileInfo.symLinkTarget());
            }
        }
        KIO::NetAccess::del(realUrl, p->widget());
        result &= KIO::NetAccess::file_copy(temporaryFile.fileName(), realUrl, p->widget());

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

        QSharedPointer<const Entry> entry = partWidget->fileView()->currentElement().dynamicCast<const Entry>();
        if (!entry.isNull()) {
            QList<KUrl> urlList = FileInfo::entryUrls(entry.data(), partWidget->fileView()->fileModel()->bibliographyFile()->property(File::Url).toUrl(), FileInfo::TestExistanceYes);
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

    void readConfiguration() {
        /// Fetch settings from configuration
        KConfigGroup configGroup(config, Preferences::groupUserInterface);
        const Preferences::ElementDoubleClickAction doubleClickAction = (Preferences::ElementDoubleClickAction)configGroup.readEntry(Preferences::keyElementDoubleClickAction, (int)Preferences::defaultElementDoubleClickAction);

        disconnect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), partWidget->fileView(), SLOT(editElement(QSharedPointer<Element>)));
        disconnect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), p, SLOT(elementViewDocument()));
        switch (doubleClickAction) {
        case Preferences::ActionOpenEditor:
            connect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), partWidget->fileView(), SLOT(editElement(QSharedPointer<Element>)));
            break;
        case Preferences::ActionViewDocument:
            connect(partWidget->fileView(), SIGNAL(elementExecuted(QSharedPointer<Element>)), p, SLOT(elementViewDocument()));
            break;
        }
    }
};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate(this))
{
    setComponentData(KBibTeXPartFactory::componentData());
    setObjectName("KBibTeXPart::KBibTeXPart");

    d->partWidget = new PartWidget(parentWidget);
    d->partWidget->fileView()->setReadOnly(!isReadWrite());
    connect(d->partWidget->fileView(), SIGNAL(modified()), this, SLOT(setModified()));
    setWidget(d->partWidget);

    setupActions(browserViewWanted);

    /* // FIXME
    if (browserViewWanted)
        new KBibTeXBrowserExtension(this);
        */

    d->initializeNew();

    NotificationHub::registerNotificationListener(this, NotificationHub::EventConfigurationChanged);
    d->readConfiguration();

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

void KBibTeXPart::notificationEvent(int eventId)
{
    if (eventId == NotificationHub::EventConfigurationChanged)
        d->readConfiguration();
}

void KBibTeXPart::setupActions(bool /*browserViewWanted FIXME*/)
{
    d->fileSaveAction = actionCollection()->addAction(KStandardAction::Save, this, SLOT(documentSave()));
    d->fileSaveAction->setEnabled(false);
    actionCollection()->addAction(KStandardAction::SaveAs, this, SLOT(documentSaveAs()));
    KAction *saveCopyAsAction = new KAction(KIcon("document-save"), i18n("Save Copy As..."), this);
    actionCollection()->addAction("file_save_copy_as", saveCopyAsAction);
    connect(saveCopyAsAction, SIGNAL(triggered()), this, SLOT(documentSaveCopyAs()));

    KAction *filterWidgetAction = new KAction(i18n("Filter"), this);
    actionCollection()->addAction("toolbar_filter_widget", filterWidgetAction);
    filterWidgetAction->setIcon(KIcon("view-filter"));
    filterWidgetAction->setShortcut(Qt::CTRL + Qt::Key_F);
    connect(filterWidgetAction, SIGNAL(triggered()), d->partWidget->filterBar(), SLOT(setFocus()));

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
    connect(d->elementEditAction, SIGNAL(triggered()), d->partWidget->fileView(), SLOT(editCurrentElement()));
    d->elementViewDocumentAction = new KAction(KIcon("application-pdf"), i18n("View Document"), this);
    d->elementViewDocumentAction->setShortcut(Qt::CTRL + Qt::Key_D);
    actionCollection()->addAction(QLatin1String("element_viewdocument"),  d->elementViewDocumentAction);
    connect(d->elementViewDocumentAction, SIGNAL(triggered()), this, SLOT(elementViewDocument()));

    d->elementFindPDFAction = new KAction(KIcon("application-pdf"), i18n("Find PDF..."), this);
    actionCollection()->addAction(QLatin1String("element_findpdf"),  d->elementFindPDFAction);
    connect(d->elementFindPDFAction, SIGNAL(triggered()), this, SLOT(elementFindPDF()));

    d->entryApplyDefaultFormatString = new KAction(KIcon("favorites"), i18n("Format entry ids"), this);
    actionCollection()->addAction(QLatin1String("entry_applydefaultformatstring"), d->entryApplyDefaultFormatString);
    connect(d->entryApplyDefaultFormatString, SIGNAL(triggered()), this, SLOT(applyDefaultFormatString()));

    Clipboard *clipboard = new Clipboard(d->partWidget->fileView());

    d->editCopyReferencesAction = new KAction(KIcon("edit-copy"), i18n("Copy References"), this);
    d->editCopyReferencesAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_C);
    actionCollection()->addAction(QLatin1String("edit_copy_references"),  d->editCopyReferencesAction);
    connect(d->editCopyReferencesAction, SIGNAL(triggered()), clipboard, SLOT(copyReferences()));

    d->editDeleteAction = new KAction(KIcon("edit-table-delete-row"), i18n("Delete"), this);
    d->editDeleteAction->setShortcut(Qt::Key_Delete);
    actionCollection()->addAction(QLatin1String("edit_delete"),  d->editDeleteAction);
    connect(d->editDeleteAction, SIGNAL(triggered()), d->partWidget->fileView(), SLOT(selectionDelete()));

    d->editCutAction = actionCollection()->addAction(KStandardAction::Cut, clipboard, SLOT(cut()));
    d->editCopyAction = actionCollection()->addAction(KStandardAction::Copy, clipboard, SLOT(copy()));
    actionCollection()->addAction(QLatin1String("edit_copy_references"),  d->editCopyReferencesAction);
    d->editPasteAction = actionCollection()->addAction(KStandardAction::Paste, clipboard, SLOT(paste()));

    /// build context menu for central BibTeX file view
    d->partWidget->fileView()->setContextMenuPolicy(Qt::ActionsContextMenu);
    d->partWidget->fileView()->addAction(d->elementEditAction);
    d->partWidget->fileView()->addAction(d->elementViewDocumentAction);
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    d->partWidget->fileView()->addAction(separator);
    d->partWidget->fileView()->addAction(d->editCutAction);
    d->partWidget->fileView()->addAction(d->editCopyAction);
    d->partWidget->fileView()->addAction(d->editCopyReferencesAction);
    d->partWidget->fileView()->addAction(d->editPasteAction);
    d->partWidget->fileView()->addAction(d->editDeleteAction);
    separator = new QAction(this);
    separator->setSeparator(true);
    d->partWidget->fileView()->addAction(separator);

    // TODO

    connect(d->partWidget->fileView(), SIGNAL(selectedElementsChanged()), this, SLOT(updateActions()));
    connect(d->partWidget->fileView(), SIGNAL(currentElementChanged(QSharedPointer<Element>,File*)), this, SLOT(updateActions()));

    d->partWidget->fileView()->addAction(d->elementFindPDFAction);
    d->partWidget->fileView()->addAction(d->entryApplyDefaultFormatString);

    d->colorLabelContextMenu = new ColorLabelContextMenu(d->partWidget->fileView());
    d->colorLabelContextMenuAction = actionCollection()->addAction(QLatin1String("entry_colorlabel"), d->colorLabelContextMenu->menuAction());

    setXMLFile(RCFileName);

    d->findDuplicatesUI = new FindDuplicatesUI(this, d->partWidget->fileView());
    d->lyx = new LyX(this, d->partWidget->fileView());

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

    /// If the current file is "watchable" (i.e. a local file),
    /// memorize local filename for future reference
    const QString watchableFilename = url().isValid() && url().isLocalFile() ? url().pathOrUrl() : QString();
    /// Stop watching local file that will be written to
    if (!watchableFilename.isEmpty())
        d->fileSystemWatcher.removePath(watchableFilename);

    const bool saveOperationSuccess = d->saveFile(localFilePath());

    /// Continue watching local file after write operation
    if (!watchableFilename.isEmpty()) {
        const QStringList filesBefore = d->fileSystemWatcher.files();
        const QString toBeAdded = watchableFilename;
        d->fileSystemWatcher.addPath(toBeAdded);
        const QStringList filesAfter = d->fileSystemWatcher.files();
        kWarning() << "toBeAdded:" << toBeAdded;
        kWarning() << "files before:" << filesBefore.count() << " files after:" << filesAfter.count();
        kWarning() << "before included?" << filesBefore.contains(toBeAdded) << " after included?" << filesAfter.contains(toBeAdded);
    } else
        kWarning() << "watchableFilename is Empty";

    if (!saveOperationSuccess) {
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
    KUrl newUrl = d->getSaveFilename();
    if (!newUrl.isValid() || !d->checkOverwrite(newUrl, widget()))
        return false;

    /// Remove old URL from file system watcher
    if (url().isValid() && url().isLocalFile())
        d->fileSystemWatcher.removePath(url().pathOrUrl());
    else
        kWarning() << "KBibTeXPart::documentSaveAs: Not removing" << url().pathOrUrl() << "from fileSystemWatcher";

    if (KParts::ReadWritePart::saveAs(newUrl)) {
        kDebug() << "setting url to be " << newUrl.pathOrUrl();
        d->model->bibliographyFile()->setProperty(File::Url, newUrl);
        return true;
    } else
        return false;
}

bool KBibTeXPart::documentSaveCopyAs()
{
    d->isSaveAsOperation = true;
    KUrl newUrl = d->getSaveFilename(false);
    if (!newUrl.isValid() || !d->checkOverwrite(newUrl, widget()) || newUrl.equals(url()))
        return false;

    /// difference from KParts::ReadWritePart::saveAs:
    /// current document's URL won't be changed
    return d->saveFile(newUrl);
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
    QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
    if (mil.count() == 1) {
        QSharedPointer<Entry> entry = d->partWidget->fileView()->fileModel()->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(*mil.constBegin()).row()).dynamicCast<Entry>();
        if (!entry.isNull())
            FindPDFUI::interactiveFindPDF(*entry, *d->bibTeXFile, widget());
    }
}

void KBibTeXPart::applyDefaultFormatString()
{
    QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
    foreach(const QModelIndex &index, mil) {
        QSharedPointer<Entry> entry = d->partWidget->fileView()->fileModel()->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(index).row()).dynamicCast<Entry>();
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
    const bool success = d->openFile(url(), localFilePath());
    emit completed();
    return success;
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
    d->partWidget->fileView()->setSelectedElement(newEntry);
    if (d->partWidget->fileView()->editElement(newEntry))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
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
    d->partWidget->fileView()->setSelectedElement(newMacro);
    if (d->partWidget->fileView()->editElement(newMacro))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
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
    d->partWidget->fileView()->setSelectedElement(newPreamble);
    if (d->partWidget->fileView()->editElement(newPreamble))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
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
    d->partWidget->fileView()->setSelectedElement(newComment);
    if (d->partWidget->fileView()->editElement(newComment))
        d->partWidget->fileView()->scrollToBottom(); // FIXME always correct behaviour?
    else {
        /// Editing this new element was cancelled,
        /// therefore remove it again
        d->model->removeRow(d->model->rowCount() - 1);
    }
}

void KBibTeXPart::updateActions()
{
    bool emptySelection = d->partWidget->fileView()->selectedElements().isEmpty();
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
    if (d->partWidget->fileView()->selectionModel() != NULL) {
        QModelIndexList mil = d->partWidget->fileView()->selectionModel()->selectedRows();
        for (QModelIndexList::ConstIterator it = mil.constBegin(); it != mil.constEnd(); ++it) {
            QSharedPointer<Entry> entry = d->partWidget->fileView()->fileModel()->element(d->partWidget->fileView()->sortFilterProxyModel()->mapToSource(*it).row()).dynamicCast<Entry>();
            if (!entry.isNull())
                references << entry->id();
        }
    }
    d->lyx->setReferences(references);
}

void KBibTeXPart::fileExternallyChange(const QString &path)
{
    /// Should never happen: triggering this slot for non-local or invalid URLs
    if (!url().isValid() || !url().isLocalFile())
        return;
    /// Should never happen: triggering this slot for filenames not being the opened file
    if (path != url().pathOrUrl()) {
        kWarning() << "Got file modification warning for wrong file: " << path << "!=" << url().pathOrUrl();
        return;
    }

    /// Stop watching file while asking for user interaction
    d->fileSystemWatcher.removePath(path);

    kDebug() << "Got notification that file was changed externally:" << path;
    if (KMessageBox::warningContinueCancel(widget(), i18n("The file '%1' has changed on disk.\n\nReload file or ignore changes on disk?", path), i18n("File changed externally"), KGuiItem(i18n("Reload file"), KIcon("edit-redo")), KGuiItem(i18n("Ignore on-disk changes"), KIcon("edit-undo"))) == KMessageBox::Continue) {
        kDebug() << "  User chose to continue";
        d->openFile(KUrl::fromLocalFile(path), path);
        /// No explicit call to QFileSystemWatcher.addPath(...) necessary,
        /// openFile(...) has done that already
    } else {
        kDebug() << "  User chose to cancel reload";
        /// Even if the user did not request reloaded the file,
        /// still resume watching file for future external changes
        if (!path.isEmpty()) {
            const QStringList filesBefore = d->fileSystemWatcher.files();
            const QString toBeAdded = path;
            d->fileSystemWatcher.addPath(toBeAdded);
            const QStringList filesAfter = d->fileSystemWatcher.files();
            kWarning() << "toBeAdded:" << toBeAdded;
            kWarning() << "files before:" << filesBefore.count() << " files after:" << filesAfter.count();
            kWarning() << "before included?" << filesBefore.contains(toBeAdded) << " after included?" << filesAfter.contains(toBeAdded);
        } else
            kWarning() << "path is Empty";
    }
}
