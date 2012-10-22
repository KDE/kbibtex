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
#include <QFile>
#include <QStringList>
#include <QUrl>
#include <QTextStream>

#include <KDebug>
#include <KLocale>

#include "element.h"
#include "entry.h"
#include "fileexporterbibtex.h"
#include "fileexporterblg.h"

FileExporterBLG::FileExporterBLG()
        : FileExporterToolchain(), m_latexLanguage("english"), m_latexBibStyle("plain")
{
    m_laTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-blg.tex");
    m_bibTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-blg.bib");
}

FileExporterBLG::~FileExporterBLG()
{
    // nothing
}

void FileExporterBLG::reloadConfig()
{
    // nothing
}

bool FileExporterBLG::save(QIODevice *ioDevice, const File *bibtexfile, QStringList *errorLog)
{
    bool result = false;

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, bibtexfile, errorLog);
        bibtexExporter->save(ioDevice, bibtexfile, NULL);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateBLG(errorLog);

    return result;
}

bool FileExporterBLG::save(QIODevice *ioDevice, const QSharedPointer<const Element> element, QStringList *errorLog)
{
    bool result = false;

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, element, errorLog);
        bibtexExporter->save(ioDevice, element, NULL);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateBLG(errorLog);

    return result;
}

void FileExporterBLG::setLaTeXLanguage(const QString &language)
{
    m_latexLanguage = language;
}

void FileExporterBLG::setLaTeXBibliographyStyle(const QString &bibStyle)
{
    m_latexBibStyle = bibStyle;
}

bool FileExporterBLG::generateBLG(QStringList *errorLog)
{
    QStringList cmdLines = QStringList() << QLatin1String("pdflatex -halt-on-error bibtex-to-blg.tex") << QLatin1String("bibtex bibtex-to-blg");

    if (writeLatexFile(m_laTeXFilename) && runProcesses(cmdLines, errorLog))
        return true;
    else {
        kWarning() << "Generating BLG failed";
        return false;
    }
}

bool FileExporterBLG::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        ts.setCodec("UTF-8");
        ts << "\\documentclass{article}\n";
        ts << "\\usepackage[T1]{fontenc}\n";
        ts << "\\usepackage[utf8]{inputenc}\n";
        if (kpsewhich("babel.sty"))
            ts << "\\usepackage[" << m_latexLanguage << "]{babel}\n";
        if (kpsewhich("hyperref.sty"))
            ts << "\\usepackage[pdfproducer={KBibTeX: http://home.gna.org/kbibtex/},pdftex]{hyperref}\n";
        else if (kpsewhich("url.sty"))
            ts << "\\usepackage{url}\n";
        if (m_latexBibStyle.startsWith("apacite") && kpsewhich("apacite.sty"))
            ts << "\\usepackage[bibnewpage]{apacite}\n";
        ts << "\\bibliographystyle{" << m_latexBibStyle << "}\n";
        ts << "\\begin{document}\n";
        ts << "\\nocite{*}\n";
        ts << "\\bibliography{bibtex-to-blg}\n";
        ts << "\\end{document}\n";
        latexFile.close();
        return true;
    } else
        return false;
}
