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
#include <ktoggleaction.h>

#include <file.h>
#include <fileimporterbibtex.h>
#include <bibtexfilemodel.h>

#include "part.h"
#include "partfactory.h"
#include "browserextension.h"

static const char RCFileName[] = "simplekbibtexpartui.rc";

KBibTeXPart::KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted)
        : KParts::ReadOnlyPart(parent)
{
    setComponentData(KBibTeXPartFactory::componentData());

    // TODO Setup view
    m_view = new KBibTeX::GUI::Widgets::BibTeXFileView();
    setWidget(m_view);

    setupActions(browserViewWanted);

    if (browserViewWanted)
        new KBibTeXBrowserExtension(this);
}

KBibTeXPart::~KBibTeXPart()
{
    // nothing
}

void KBibTeXPart::setupActions(bool browserViewWanted)
{
    KActionCollection *actions = actionCollection();

    // TODO

    fitActionSettings();

    setXMLFile(RCFileName);
}


void KBibTeXPart::fitActionSettings()
{
    // TODO
}


bool KBibTeXPart::openFile()
{
    KBibTeX::IO::FileImporterBibTeX *importer = new KBibTeX::IO::FileImporterBibTeX("latex", false);
    QFile inputfile(localFilePath());
    inputfile.open(QIODevice::ReadOnly);
    KBibTeX::IO::File *bibtexFile = bibtexFile = importer->load(&inputfile);
    inputfile.close();
    delete importer;

    kDebug() << "count " << bibtexFile->count() << endl;
    KBibTeX::GUI::Widgets::BibTeXFileModel *model = new   KBibTeX::GUI::Widgets::BibTeXFileModel();
    model->setBibTeXFile(bibtexFile);

    m_view->setModel(model);

    return true;
}
