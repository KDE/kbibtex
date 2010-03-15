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

#include <KEncodingFileDialog>
#include <KMessageBox>
#include <KDebug>
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
    KBibTeX::GUI::BibTeXEditor *editor;
    KBibTeX::GUI::Widgets::BibTeXFileModel *model;
    KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel *sortFilterProxyModel;
    KBibTeX::GUI::Widgets::FilterBar *filterBar;

    KBibTeXPartPrivate()
            : sortFilterProxyModel(NULL) {
        // nothing
    }

    KBibTeX::IO::FileImporter *fileImporterFactory(const KUrl& url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "ris") {
            kDebug() << "Selecting KBibTeX::IO::FileImporterRIS" << endl;
            return new KBibTeX::IO::FileImporterRIS();
        } else {
            kDebug() << "Selecting KBibTeX::IO::FileImporterBibTeX" << endl;
            return new KBibTeX::IO::FileImporterBibTeX("latex", false);
        }
    }

    KBibTeX::IO::FileExporter *fileExporterFactory(const KUrl& url) {
        QString ending = url.path().toLower();
        int p = ending.lastIndexOf(".");
        ending = ending.mid(p + 1);

        if (ending == "html") {
            kDebug() << "Selecting KBibTeX::IO::FileExporterXSLT" << endl;
            return new KBibTeX::IO::FileExporterXSLT();
        } else if (ending == "xml") {
            kDebug() << "Selecting KBibTeX::IO::FileExporterXML" << endl;
            return new KBibTeX::IO::FileExporterXML();
        } else if (ending == "ris") {
            kDebug() << "Selecting KBibTeX::IO::FileExporterRIS" << endl;
            return new KBibTeX::IO::FileExporterRIS();
        } else if (ending == "html" || ending == "html") {
            kDebug() << "Selecting KBibTeX::IO::FileExporterBibTeX2HTML" << endl;
            return new KBibTeX::IO::FileExporterBibTeX2HTML();
        } else {
            kDebug() << "Selecting KBibTeX::IO::FileExporterBibTeX" << endl;
            return new KBibTeX::IO::FileExporterBibTeX();
        }
    }
};

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadWritePart(parent), d(new KBibTeXPartPrivate())
{
    setComponentData(KBibTeXPartFactory::componentData());
    setObjectName("KBibTeXPart::KBibTeXPart");

    // TODO Setup view
    d->editor = new KBibTeX::GUI::BibTeXEditor(parentWidget);
    setWidget(d->editor);

    d->model = new KBibTeX::GUI::Widgets::BibTeXFileModel();
    d->editor->setModel(d->model);

    connect(d->editor, SIGNAL(elementExecuted(KBibTeX::IO::Element*)), d->editor, SLOT(editElement(KBibTeX::IO::Element*)));

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

    d->filterBar = new KBibTeX::GUI::Widgets::FilterBar(0);
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

    KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel *model = dynamic_cast<KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel *>(d->editor->model());
    Q_ASSERT(model != NULL);
    KBibTeX::IO::FileExporter *exporter = d->fileExporterFactory(url());
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
    KEncodingFileDialog::Result loadResult = KEncodingFileDialog::getSaveUrlAndEncoding(QString(), ":save", QLatin1String("text/x-bibtex application/xml text/html"), widget());
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


    KBibTeX::IO::FileImporter *importer = d->fileImporterFactory(url());
    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    KBibTeX::IO::File *bibtexFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    d->model->setBibTeXFile(bibtexFile);
    d->editor->setModel(d->model);
    if (d->sortFilterProxyModel != NULL) delete d->sortFilterProxyModel;
    d->sortFilterProxyModel = new KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel(this);
    d->sortFilterProxyModel->setSourceModel(d->model);
    d->editor->setModel(d->sortFilterProxyModel);
    connect(d->filterBar, SIGNAL(filterChanged(KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel::FilterQuery)), d->sortFilterProxyModel, SLOT(updateFilter(KBibTeX::GUI::Widgets::SortFilterBibTeXFileModel::FilterQuery)));

    qApp->restoreOverrideCursor();

    emit completed();
    emit setWindowCaption(url().prettyUrl());
    return true;
}
