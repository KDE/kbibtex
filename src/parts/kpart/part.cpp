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
#include "filterbar.h"

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
        kDebug() << " Initializing with empty data";
        model = new BibTeXFileModel();
        model->setBibTeXFile(new File());

        if (sortFilterProxyModel != NULL) delete sortFilterProxyModel;
        sortFilterProxyModel = new SortFilterBibTeXFileModel(p);
        sortFilterProxyModel->setSourceModel(model);
        editor->setModel(sortFilterProxyModel);
        connect(filterBar, SIGNAL(filterChanged(SortFilterBibTeXFileModel::FilterQuery)), sortFilterProxyModel, SLOT(updateFilter(SortFilterBibTeXFileModel::FilterQuery)));
    }
};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate(this))
{
    setComponentData(KBibTeXPartFactory::componentData());
    setObjectName("KBibTeXPart::KBibTeXPart");

    // TODO Setup view
    d->editor = new BibTeXEditor(parentWidget);
    setWidget(d->editor);

    connect(d->editor, SIGNAL(elementExecuted(Element*)), d->editor, SLOT(editElement(Element*)));
    connect(d->editor, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(editorKeyPressed(QKeyEvent*)));

    setupActions(browserViewWanted);

    /* // FIXME
    if (browserViewWanted)
        new KBibTeXBrowserExtension(this);
        */

    d->initializeNew();
}

KBibTeXPart::~KBibTeXPart()
{
    // nothing
}

void KBibTeXPart::setupActions(bool /*browserViewWanted FIXME*/)
{
    actionCollection()->addAction(KStandardAction::Save, this, SLOT(save()));
    actionCollection()->addAction(KStandardAction::SaveAs, this, SLOT(saveDocumentDialog()));

    d->filterBar = new FilterBar(0);
    KAction *filterWidgetAction = new KAction(this);
    actionCollection()->addAction("toolbar_filter_widget", filterWidgetAction);
    filterWidgetAction->setText(i18n("Filter"));
    filterWidgetAction->setShortcut(Qt::CTRL + Qt::Key_F);
    filterWidgetAction->setDefaultWidget(d->filterBar);
    connect(filterWidgetAction, SIGNAL(triggered()), d->filterBar, SLOT(setFocus()));

    KActionMenu *newElementAction = new KActionMenu(KIcon("address-book-new"), i18n("New element"), this);
    actionCollection()->addAction("element_new", newElementAction);
    newElementAction->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_N);
    connect(newElementAction, SIGNAL(triggered()), this, SLOT(newEntryTriggered()));
    KMenu *newElementMenu = new KMenu(newElementAction->text(), widget());
    newElementAction->setMenu(newElementMenu);
    QAction *newElement = newElementMenu->addAction(KIcon("address-book-new"), i18n("New entry"));
    connect(newElement, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newElement, smEntry);
    newElement = newElementMenu->addAction(KIcon("address-book-new"), i18n("New comment"));
    connect(newElement, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newElement, smComment);
    newElement = newElementMenu->addAction(KIcon("address-book-new"), i18n("New macro"));
    connect(newElement, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newElement, smMacro);
    newElement = newElementMenu->addAction(KIcon("address-book-new"), i18n("New preamble"));
    connect(newElement, SIGNAL(triggered()), d->signalMapperNewElement, SLOT(map()));
    d->signalMapperNewElement->setMapping(newElement, smPreamble);
    connect(d->signalMapperNewElement, SIGNAL(mapped(int)), this, SLOT(newElementTriggered(int)));

    Clipboard *clipboard = new Clipboard(d->editor);
    actionCollection()->addAction(KStandardAction::Cut, clipboard, SLOT(cut()));
    actionCollection()->addAction(KStandardAction::Copy, clipboard, SLOT(copy()));
    actionCollection()->addAction(KStandardAction::Paste, clipboard, SLOT(paste()));

    // TODO

    fitActionSettings();

    setXMLFile(RCFileName);
}

bool KBibTeXPart::saveFile()
{
    if (!isReadWrite())
        return false; //< if part is in read-only mode, then forbid any write operation
    if (!url().isLocalFile()) { // FIXME: non-local files must be handled
        KMessageBox::information(widget(), i18n("Can only save to local files"), i18n("Saving file"));
        return false;
    }
    setLocalFilePath(url().path());

    qApp->setOverrideCursor(Qt::WaitCursor);

    SortFilterBibTeXFileModel *model = dynamic_cast<SortFilterBibTeXFileModel *>(d->editor->model());
    Q_ASSERT(model != NULL);
    FileExporter *exporter = d->fileExporterFactory(url());
    QFile outputfile(localFilePath());
    outputfile.open(QIODevice::WriteOnly);
    exporter->save(&outputfile, model->bibTeXSourceModel()->bibTeXFile());
    outputfile.close();
    delete exporter;

    qApp->restoreOverrideCursor();

    emit completed();
    emit setWindowCaption(url().prettyUrl());
    return true;
}

void KBibTeXPart::saveDocumentDialog()
{
    QString startDir = QString();// QLatin1String(":save"); // FIXME: Does not work yet
    QString supportedMimeTypes = QLatin1String("text/x-bibtex application/xml");
    if (FileExporterToolchain::kpsewhich(QLatin1String("embedfile.sty")))
        supportedMimeTypes += QLatin1String(" application/pdf");
    // TODO application/x-research-info-systems application/x-endnote-refer
    supportedMimeTypes += QLatin1String(" text/html");

    KEncodingFileDialog::Result loadResult = KEncodingFileDialog::getSaveUrlAndEncoding(QString(), startDir, supportedMimeTypes, widget());
    if (!loadResult.URLs.isEmpty()) {
        KUrl url = loadResult.URLs.first();
        if (!url.isEmpty()) {
            setUrl(url);
            saveFile();
        }
    }
}

void KBibTeXPart::fitActionSettings()
{
    // TODO
}

bool KBibTeXPart::openFile()
{
    kDebug() << "Opening file: " << url();
    qApp->setOverrideCursor(Qt::WaitCursor);

    setObjectName("KBibTeXPart::KBibTeXPart for " + url().prettyUrl());

    FileImporter *importer = d->fileImporterFactory(url());
    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    File *bibtexFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    if (bibtexFile == NULL) {
        kWarning() << "Opening file failed: " << url();
        return false;
    }

    d->model->setBibTeXFile(bibtexFile);
    d->editor->setModel(d->model);
    if (d->sortFilterProxyModel != NULL) delete d->sortFilterProxyModel;
    d->sortFilterProxyModel = new SortFilterBibTeXFileModel(this);
    d->sortFilterProxyModel->setSourceModel(d->model);
    d->editor->setModel(d->sortFilterProxyModel);
    connect(d->filterBar, SIGNAL(filterChanged(SortFilterBibTeXFileModel::FilterQuery)), d->sortFilterProxyModel, SLOT(updateFilter(SortFilterBibTeXFileModel::FilterQuery)));

    qApp->restoreOverrideCursor();

    emit completed();
    emit setWindowCaption(url().prettyUrl());
    return true;
}

void KBibTeXPart::editorKeyPressed(QKeyEvent *event)
{
    // FIXME: Make key sequences configurable (QAction/KAction)
    if (event->modifiers() == Qt::NoModifier && event->matches(QKeySequence::Delete)) {
        /// delete the selected elements
        d->editor->selectionDelete();
    }
}

void KBibTeXPart::newElementTriggered(int event)
{
    switch (event) {
    case smComment:
        kWarning() << "Not yet implemented";
        // FIXME to be implemented
        break;
    case smMacro:
        kWarning() << "Not yet implemented";
        // FIXME to be implemented
        break;
    case smPreamble:
        kWarning() << "Not yet implemented";
        // FIXME to be implemented
        break;
    default:
        newEntryTriggered();
    }
}

void KBibTeXPart::newEntryTriggered()
{
    Entry *newEntry = new Entry(QLatin1String("Article"), d->findUnusedId());
    d->model->insertRow(newEntry, d->model->rowCount());
}
