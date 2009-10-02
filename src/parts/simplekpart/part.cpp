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

#include <klocale.h>
#include <kdebug.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <kselectaction.h>
#include <KToggleAction>

#include <file.h>
#include <fileimporterbibtex.h>
#include <fileimporterris.h>
#include <bibtexfilemodel.h>

#include "part.h"
#include "partfactory.h"
#include "browserextension.h"

static const char RCFileName[] = "simplekbibtexpartui.rc";

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadWritePart(parent)
{
    setComponentData(KBibTeXPartFactory::componentData());

    // TODO Setup view
    m_widget = new KBibTeX::GUI::BibTeXEditor(parentWidget);
    setWidget(m_widget);

    connect(m_widget, SIGNAL(elementExecuted(const KBibTeX::IO::Element*)), m_widget, SLOT(viewElement(const KBibTeX::IO::Element*)));

    setupActions(browserViewWanted);

    if (browserViewWanted)
        new KBibTeXBrowserExtension(this);
}

KBibTeXPart::~KBibTeXPart()
{
    // nothing
}

void KBibTeXPart::setupActions(bool /*browserViewWanted FIXME*/)
{
    // KActionCollection *actions = actionCollection(); // FIXME

    // TODO

    fitActionSettings();

    setXMLFile(RCFileName);
}

bool KBibTeXPart::saveFile()
{
    // TODO
    return false;
}

void KBibTeXPart::fitActionSettings()
{
    // TODO
}


bool KBibTeXPart::openFile()
{
    QString ending = localFilePath().toLower();
    int p = ending.lastIndexOf(".");
    ending = ending.mid(p + 1);

    KBibTeX::IO::FileImporter *importer = NULL;
    if (ending == "ris") {
        kDebug() << "Selecting KBibTeX::IO::FileImporterRIS" << endl;
        importer = new KBibTeX::IO::FileImporterRIS();
    } else {
        kDebug() << "Selecting KBibTeX::IO::FileImporterBibTeX" << endl;
        importer = new KBibTeX::IO::FileImporterBibTeX("latex", false);
    }

    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    KBibTeX::IO::File *bibtexFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    kDebug() << "count " << bibtexFile->count() << endl;
    KBibTeX::GUI::Widgets::BibTeXFileModel *model = new   KBibTeX::GUI::Widgets::BibTeXFileModel();
    model->setBibTeXFile(bibtexFile);

    m_widget->setModel(model);

    return true;
}
