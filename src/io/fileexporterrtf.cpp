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

#include "fileexporterrtf.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QDir>

#include <KSharedConfig>
#include <KConfigGroup>

#include "element.h"
#include "fileexporterbibtex.h"
#include "kbibtexnamespace.h"
#include "logging_io.h"

FileExporterRTF::FileExporterRTF()
        : FileExporterToolchain()
{
    m_fileBasename = QLatin1String("bibtex-to-rtf");
    m_fileStem = tempDir.path() + QDir::separator() + m_fileBasename;

    reloadConfig();
}

FileExporterRTF::~FileExporterRTF()
{
    // nothing
}

void FileExporterRTF::reloadConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("kbibtexrc"));
    KConfigGroup configGroup(config, QLatin1String("FileExporterPDFPS"));
    m_babelLanguage = configGroup.readEntry(keyBabelLanguage, defaultBabelLanguage);
    m_bibliographyStyle = configGroup.readEntry(keyBibliographyStyle, defaultBibliographyStyle);

    KConfigGroup configGroupGeneral(config, QLatin1String("General"));
    m_paperSize = configGroupGeneral.readEntry(keyPaperSize, defaultPaperSize);
}

bool FileExporterRTF::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
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
        result = generateRTF(iodevice, errorLog);

    iodevice->close();
    return result;
}

bool FileExporterRTF::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
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
        result = generateRTF(iodevice, errorLog);

    iodevice->close();
    return result;
}

bool FileExporterRTF::generateRTF(QIODevice *iodevice, QStringList *errorLog)
{
    QStringList cmdLines = QStringList() << QLatin1String("latex -halt-on-error bibtex-to-rtf.tex") << QLatin1String("bibtex bibtex-to-rtf") << QLatin1String("latex -halt-on-error bibtex-to-rtf.tex") << QString(QLatin1String("latex2rtf -i %1 bibtex-to-rtf.tex")).arg(m_babelLanguage);

    return writeLatexFile(m_fileStem + KBibTeX::extensionTeX) && runProcesses(cmdLines, errorLog) && writeFileToIODevice(m_fileStem + KBibTeX::extensionRTF, iodevice, errorLog);
}

bool FileExporterRTF::writeLatexFile(const QString &filename)
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
        if (m_bibliographyStyle == QLatin1String("dcu") && kpsewhich("harvard.sty") && kpsewhich("html.sty"))
            ts << "\\usepackage{html}" << endl << "\\usepackage[dcucite]{harvard}" << endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << endl;
        if (kpsewhich("geometry.sty"))
            ts << "\\usepackage[paper=" << m_paperSize << (m_paperSize.length() <= 2 ? "paper" : "") << "]{geometry}" << endl;
        ts << "\\bibliographystyle{" << m_bibliographyStyle << "}" << endl;
        ts << "\\begin{document}" << endl;
        ts << "\\nocite{*}" << endl;
        ts << "\\bibliography{bibtex-to-rtf}" << endl;
        ts << "\\end{document}" << endl;
        latexFile.close();
        return true;
    }

    return false;
}
