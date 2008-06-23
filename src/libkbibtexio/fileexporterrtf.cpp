/***************************************************************************
*   Copyright (C) 2004-2008 by Thomas Fischer                             *
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
#include <QTextStream>

#include <element.h>
#include <fileexporterbibtex.h>
#include "fileexporterrtf.h"

using namespace KBibTeX::IO;

FileExporterRTF::FileExporterRTF(const QString& latexBibStyle, const QString& latexLanguage) : FileExporterToolchain(), m_latexLanguage(latexLanguage), m_latexBibStyle(latexBibStyle)
{
    laTeXFilename = QString(workingDir).append("/bibtex-to-rtf.tex");
    bibTeXFilename = QString(workingDir).append("/bibtex-to-rtf.bib");
    outputFilename = QString(workingDir).append("/bibtex-to-rtf.rtf");
}

FileExporterRTF::~FileExporterRTF()
{
    // nothing
}

bool FileExporterRTF::save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog)
{
    m_mutex.lock();
    bool result = FALSE;

    QFile bibtexFile(bibTeXFilename);
    if (bibtexFile.open(QIODevice::WriteOnly)) {
        FileExporter * bibtexExporter = new FileExporterBibTeX();
        result = bibtexExporter->save(&bibtexFile, bibtexfile, errorLog);
        bibtexFile.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateRTF(iodevice, errorLog);

    m_mutex.unlock();
    return result;
}

bool FileExporterRTF::save(QIODevice* iodevice, const Element* element, QStringList *errorLog)
{
    m_mutex.lock();
    bool result = FALSE;

    QFile bibtexFile(bibTeXFilename);
    if (bibtexFile.open(QIODevice::WriteOnly)) {
        FileExporter * bibtexExporter = new FileExporterBibTeX();
        result = bibtexExporter->save(&bibtexFile, element, errorLog);
        bibtexFile.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateRTF(iodevice, errorLog);

    m_mutex.unlock();
    return result;
}

bool FileExporterRTF::generateRTF(QIODevice* iodevice, QStringList *errorLog)
{
    QStringList cmdLines;
    cmdLines << "latex bibtex-to-rtf.tex" << "bibtex bibtex-to-rtf" << "latex bibtex-to-rtf.tex" << "latex2rtf bibtex-to-rtf.tex";

    if (writeLatexFile(laTeXFilename) && runProcesses(cmdLines, errorLog) && writeFileToIODevice(outputFilename, iodevice))
        return TRUE;
    else
        return FALSE;
}

bool FileExporterRTF::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        ts.setCodec("latin1");
        ts << "\\documentclass{article}\n";
        if (kpsewhich("t2aenc.dfu") &&  kpsewhich("t1enc.dfu"))
            ts << "\\usepackage[T1,T2A]{fontenc}\n";
        ts << "\\usepackage[latin1]{inputenc}\n";
        ts << "\\usepackage[" << m_latexLanguage << "]{babel}\n";
        if (kpsewhich("hyperref.sty"))
            ts << "\\usepackage[pdfproducer={KBibTeX: http://www.t-fischer.net/kbibtex/},pdftex]{hyperref}\n";
        else if (kpsewhich("url.sty"))
            ts << "\\usepackage{url}\n";
        if (m_latexBibStyle.startsWith("apacite") && kpsewhich("apacite.sty"))
            ts << "\\usepackage[bibnewpage]{apacite}\n";
        ts << "\\bibliographystyle{" << m_latexBibStyle << "}\n";
        ts << "\\begin{document}\n";
        ts << "\\nocite{*}\n";
        ts << "\\bibliography{bibtex-to-rtf}\n";
        ts << "\\end{document}\n";
        latexFile.close();
        return TRUE;
    } else
        return FALSE;
}
