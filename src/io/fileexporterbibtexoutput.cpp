/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#include <QBuffer>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <QUrl>
#include <QTextStream>

#include <KDebug>
#include <KLocale>

#include "element.h"
#include "entry.h"
#include "fileexporterbibtex.h"
#include "fileexporterbibtexoutput.h"

const QString extensionTeX = QLatin1String(".tex");
const QString extensionAux = QLatin1String(".aux");
const QString extensionBBL = QLatin1String(".bbl");
const QString extensionBLG = QLatin1String(".blg");

FileExporterBibTeXOutput::FileExporterBibTeXOutput(OutputType outputType)
        : FileExporterToolchain(), m_outputType(outputType), m_latexLanguage("english"), m_latexBibStyle("plain")
{
    m_fileBasename = QLatin1String("bibtex-to-output");
    m_fileStem = tempDir.name() + QDir::separator() + m_fileBasename;
}

FileExporterBibTeXOutput::~FileExporterBibTeXOutput()
{
    // nothing
}

void FileExporterBibTeXOutput::reloadConfig()
{
    // nothing
}

bool FileExporterBibTeXOutput::save(QIODevice *ioDevice, const File *bibtexfile, QStringList *errorLog)
{
    bool result = false;

    QBuffer buffer(this);
    if (buffer.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&buffer, bibtexfile, errorLog);
        buffer.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateOutput(errorLog);

    if (result)
        result = writeFileToIODevice(m_fileStem + (m_outputType == BibTeXLogFile ? extensionBLG : extensionBBL), ioDevice, errorLog);

    return result;
}

bool FileExporterBibTeXOutput::save(QIODevice *ioDevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    bool result = false;

    QBuffer buffer(this);
    if (buffer.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&buffer, element, bibtexfile, errorLog);
        buffer.close();
        delete bibtexExporter;
    }

    if (result)
        result = generateOutput(errorLog);

    if (result)
        result = writeFileToIODevice(m_fileStem + (m_outputType == BibTeXLogFile ? extensionBLG : extensionBBL), ioDevice, errorLog);

    return result;
}

void FileExporterBibTeXOutput::setLaTeXLanguage(const QString &language)
{
    m_latexLanguage = language;
}

void FileExporterBibTeXOutput::setLaTeXBibliographyStyle(const QString &bibStyle)
{
    m_latexBibStyle = bibStyle;
}

bool FileExporterBibTeXOutput::generateOutput(QStringList *errorLog)
{
    QStringList cmdLines = QStringList() << QLatin1String("pdflatex -halt-on-error ") + m_fileBasename + extensionTeX << QLatin1String("bibtex ") + m_fileBasename + extensionAux;

    if (writeLatexFile(m_fileStem + extensionTeX) && runProcesses(cmdLines, errorLog))
        return true;
    else {
        kWarning() << "Generating BibTeX output failed";
        return false;
    }
}

bool FileExporterBibTeXOutput::writeLatexFile(const QString &filename)
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
        ts << QLatin1String("\\bibliography{") + m_fileBasename + QLatin1String("}\n");
        ts << "\\end{document}\n";
        latexFile.close();
        return true;
    } else
        return false;
}
