/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "fileexporterps.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QDir>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KDebug>

#include "element.h"
#include "fileexporterbibtex.h"
#include "kbibtexnamespace.h"

FileExporterPS::FileExporterPS()
        : FileExporterToolchain()
{
    m_fileBasename = QLatin1String("bibtex-to-ps");
    m_fileStem = tempDir.name() + QDir::separator() + m_fileBasename;

    reloadConfig();
}

FileExporterPS::~FileExporterPS()
{
    // nothing
}

void FileExporterPS::reloadConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("kbibtexrc"));
    KConfigGroup configGroup(config, QLatin1String("FileExporterPDFPS"));
    m_babelLanguage = configGroup.readEntry(keyBabelLanguage, defaultBabelLanguage);
    m_bibliographyStyle = configGroup.readEntry(keyBibliographyStyle, defaultBibliographyStyle);

    KConfigGroup configGroupGeneral(config, QLatin1String("General"));
    m_paperSize = configGroupGeneral.readEntry(keyPaperSize, defaultPaperSize);
    m_font = configGroupGeneral.readEntry(keyFont, defaultFont);
}

bool FileExporterPS::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        kDebug() << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("latex"));
        result = bibtexExporter->save(&output, bibtexfile, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generatePS(iodevice, errorLog);

    iodevice->close();
    return result;
}

bool FileExporterPS::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        kDebug() << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("latex"));
        result = bibtexExporter->save(&output, element, bibtexfile, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generatePS(iodevice, errorLog);

    iodevice->close();
    return result;
}

bool FileExporterPS::generatePS(QIODevice *iodevice, QStringList *errorLog)
{
    QStringList cmdLines = QStringList() << QLatin1String("latex -halt-on-error bibtex-to-ps.tex") << QLatin1String("bibtex bibtex-to-ps") << QLatin1String("latex -halt-on-error bibtex-to-ps.tex") << QLatin1String("latex -halt-on-error bibtex-to-ps.tex") << QLatin1String("dvips -R2 -o bibtex-to-ps.ps bibtex-to-ps.dvi");

    return writeLatexFile(m_fileStem + KBibTeX::extensionTeX) && runProcesses(cmdLines, errorLog) && beautifyPostscriptFile(m_fileStem + KBibTeX::extensionPostScript, "Exported Bibliography") && writeFileToIODevice(m_fileStem + KBibTeX::extensionPostScript, iodevice, errorLog);
}

bool FileExporterPS::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        ts.setCodec("UTF-8");
        ts << "\\documentclass{article}" << endl;
        ts << "\\usepackage[T1]{fontenc}" << endl;
        ts << "\\usepackage[utf8]{inputenc}" << endl;
        if (kpsewhich("babel.sty"))
            ts << "\\usepackage[" << m_babelLanguage << "]{babel}" << endl;
        if (kpsewhich("url.sty"))
            ts << "\\usepackage{url}" << endl;
        if (m_bibliographyStyle.startsWith(QLatin1String("apacite")) && kpsewhich("apacite.sty"))
            ts << "\\usepackage[bibnewpage]{apacite}" << endl;
        if ((m_bibliographyStyle == QLatin1String("agsm") || m_bibliographyStyle == QLatin1String("dcu") || m_bibliographyStyle == QLatin1String("jmr") || m_bibliographyStyle == QLatin1String("jphysicsB") || m_bibliographyStyle == QLatin1String("kluwer") || m_bibliographyStyle == QLatin1String("nederlands") || m_bibliographyStyle == QLatin1String("dcu") || m_bibliographyStyle == QLatin1String("dcu")) && kpsewhich("harvard.sty") && kpsewhich("html.sty"))
            ts << "\\usepackage{html}" << endl << "\\usepackage[dcucite]{harvard}" << endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << endl;
        if (kpsewhich("geometry.sty"))
            ts << "\\usepackage[paper=" << m_paperSize << (m_paperSize.length() <= 2 ? "paper" : "") << "]{geometry}" << endl;
        if (!m_font.isEmpty() && kpsewhich(m_font + QLatin1String(".sty")))
            ts << "\\usepackage{" << m_font << "}" << endl;
        ts << "\\bibliographystyle{" << m_bibliographyStyle << "}" << endl;
        ts << "\\begin{document}" << endl;
        ts << "\\nocite{*}" << endl;
        ts << "\\bibliography{bibtex-to-ps}" << endl;
        ts << "\\end{document}" << endl;
        latexFile.close();
        return true;
    } else
        return false;
}

bool FileExporterPS::beautifyPostscriptFile(const QString &filename, const QString &title)
{
    QFile postscriptFile(filename);
    if (postscriptFile.open(QFile::ReadOnly)) {
        QTextStream ts(&postscriptFile);
        QStringList lines;
        QString line;
        int i = 0;
        while (!(line = ts.readLine()).isNull()) {
            if (i < 32 && line.startsWith(QLatin1String("%%Title:")))
                line = QLatin1String("%%Title: ") + title;
            else if (i < 32 && line.startsWith(QLatin1String("%%Creator:")))
                line += QLatin1String("; exported from within KBibTeX: http://home.gna.org/kbibtex/");
            lines += line;
            ++i;
        }
        postscriptFile.close();

        if (postscriptFile.open(QFile::WriteOnly)) {
            QTextStream ts(&postscriptFile);
            foreach(const QString &line, lines) ts << line << endl;
            postscriptFile.close();
        } else
            return false;
    } else
        return false;

    return true;
}
