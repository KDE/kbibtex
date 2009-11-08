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

#include <KEncodingFileDialog>
#include <klocale.h>
#include <kdebug.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kselectaction.h>
#include <KToggleAction>

#include <file.h>
#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>
#include <fileimporterris.h>
#include <fileexporterris.h>
#include <bibtexfilemodel.h>

#include "part.h"
#include "partfactory.h"
// #include "browserextension.h" // FIXME

static const char RCFileName[] = "simplekbibtexpartui.rc";

class KBibTeXPart::KBibTeXPartPrivate
{
public:
    KBibTeX::GUI::BibTeXEditor *m_widget;

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

        if (ending == "ris") {
            kDebug() << "Selecting KBibTeX::IO::FileExporterRIS" << endl;
            return new KBibTeX::IO::FileExporterRIS();
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

    // TODO Setup view
    d->m_widget = new KBibTeX::GUI::BibTeXEditor(parentWidget);
    setWidget(d->m_widget);

    connect(d->m_widget, SIGNAL(elementExecuted(const KBibTeX::IO::Element*)), d->m_widget, SLOT(viewElement(const KBibTeX::IO::Element*)));

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

    // TODO

    fitActionSettings();

    setXMLFile(RCFileName);
}

bool KBibTeXPart::saveFile()
{
    if (!isReadWrite())
        return false; //< if part is in read-only mode, then forbid any write operation

    qApp->setOverrideCursor(Qt::WaitCursor);

    KBibTeX::IO::FileExporter *exporter = d->fileExporterFactory(url());
    QFile outputfile(localFilePath());
    outputfile.open(QIODevice::WriteOnly);
    KBibTeX::GUI::Widgets::BibTeXFileModel *model = dynamic_cast<KBibTeX::GUI::Widgets::BibTeXFileModel *>(d->m_widget->model());
    exporter->save(&outputfile, model->bibTeXFile());
    outputfile.close();
    delete exporter;

    qApp->restoreOverrideCursor();
    return true;
}

void KBibTeXPart::saveDocumentDialog()
{
    KEncodingFileDialog::Result loadResult = KEncodingFileDialog::getSaveUrlAndEncoding(QString(), ":save", QString(), widget());
    KUrl url = loadResult.URLs.first();
    if (!url.isEmpty()) {
        setUrl(url);
        saveFile();
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

    KBibTeX::IO::FileImporter *importer = d->fileImporterFactory(url());
    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    KBibTeX::IO::File *bibtexFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    KBibTeX::GUI::Widgets::BibTeXFileModel *model = new KBibTeX::GUI::Widgets::BibTeXFileModel();
    model->setBibTeXFile(bibtexFile);

    d->m_widget->setModel(model);

    qApp->restoreOverrideCursor();
    return true;
}
