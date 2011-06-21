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
#include <QFile>
#include <QStringList>
#include <QTextStream>

#include <element.h>
#include <fileexporterbibtex.h>
#include "fileexporterrtf.h"

FileExporterRTF::FileExporterRTF(const QString& latexBibStyle, const QString& latexLanguage)
        : FileExporterToolchain(), m_latexLanguage(latexLanguage), m_latexBibStyle(latexBibStyle)
{
    m_laTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-rtf.tex");
    m_bibTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-rtf.bib");
    m_outputFilename = tempDir.name() + QLatin1String("/bibtex-to-rtf.rtf");
}

FileExporterRTF::~FileExporterRTF()
{
    // nothing
}

bool FileExporterRTF::save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog)
{
    bool result = false;

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX * bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, bibtexfile, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateRTF(iodevice, errorLog);

    return result;
}

bool FileExporterRTF::save(QIODevice* iodevice, const Element* element, QStringList *errorLog)
{
    bool result = false;

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX * bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, element, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateRTF(iodevice, errorLog);

    // m_mutex.unlock(); // FIXME: required?
    return result;
}

bool FileExporterRTF::generateRTF(QIODevice* iodevice, QStringList *errorLog)
{
    QStringList cmdLines = QStringList() << QLatin1String("latex -halt-on-error bibtex-to-rtf.tex") << QLatin1String("bibtex bibtex-to-rtf") << QLatin1String("latex -halt-on-error bibtex-to-rtf.tex") << QLatin1String("latex2rtf bibtex-to-rtf.tex");

    if (writeLatexFile(m_laTeXFilename) && runProcesses(cmdLines, errorLog) && writeFileToIODevice(m_outputFilename, iodevice))
        return TRUE;
    else
        return FALSE;
}

bool FileExporterRTF::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        ts.setCodec("UTF-8");
        ts << "\\documentclass{article}\n";
        if (kpsewhich("t2aenc.dfu") &&  kpsewhich("t1enc.dfu"))
            ts << "\\usepackage[T1,T2A]{fontenc}\n";
        ts << "\\usepackage[utf8]{inputenc}\n";
        if (kpsewhich("babel.sty"))
            ts << "\\usepackage[" << m_latexLanguage << "]{babel}\n";
        if (kpsewhich("url.sty"))
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
