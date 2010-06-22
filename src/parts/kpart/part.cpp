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

#include <bibtexentries.h>
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

#include "part.h"
#include "partfactory.h"
// #include "browserextension.h" // FIXME

static const char RCFileName[] = "kbibtexpartui.rc";

class KBibTeXPart::KBibTeXPartPrivate
{
public:
    BibTeXEditor *editor;
    BibTeXFileModel *model;
    SortFilterBibTeXFileModel *sortFilterProxyModel;
    FilterBar *filterBar;

    KBibTeXPartPrivate()
            : sortFilterProxyModel(NULL) {
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
};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate())
{
    setComponentData(KBibTeXPartFactory::componentData());
    setObjectName("KBibTeXPart::KBibTeXPart");

    // TODO Setup view
    d->editor = new BibTeXEditor(parentWidget);
    setWidget(d->editor);

    d->model = new BibTeXFileModel();
    d->editor->setModel(d->model);

    connect(d->editor, SIGNAL(elementExecuted(Element*)), d->editor, SLOT(editElement(Element*)));
    connect(d->editor, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(editorKeyPressed(QKeyEvent*)));

    setupActions(browserViewWanted);

    /* // FIXME
    if (browserViewWanted)
        new KBibTeXBrowserExtension(this);
        */
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
    KMenu *newElementMenu = new KMenu(newElementAction->text(), widget());
    newElementAction->setMenu(newElementMenu);
    newElementMenu->addAction(KIcon("address-book-new"), i18n("New entry"));
    newElementMenu->addAction(KIcon("address-book-new"), i18n("New comment"));
    newElementMenu->addAction(KIcon("address-book-new"), i18n("New macro"));
    newElementMenu->addAction(KIcon("address-book-new"), i18n("New preamble"));

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
    qApp->setOverrideCursor(Qt::WaitCursor);

    kDebug() << "Opening URL " << url().prettyUrl() << endl;

    setObjectName("KBibTeXPart::KBibTeXPart for " + url().prettyUrl());


    FileImporter *importer = d->fileImporterFactory(url());
    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    File *bibtexFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    if (bibtexFile == NULL) {
        kWarning() << "Opening file failed";
        return false;
    }

    BibTeXEntries::self()->format(*bibtexFile, KBibTeX::cCamelCase);

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
    if (event->modifiers() == Qt::NoModifier && event->matches(QKeySequence::Delete)) { // FIXME: Make key sequence configurable
        /// delete the current (selected) element
        d->model->removeRow(d->editor->currentIndex().row());
    }
}
