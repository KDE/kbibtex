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
#include <QDir>

#include <KDebug>
#include <KLocale>
#include <KSharedConfig>
#include <KConfigGroup>

#include <fileinfo.h>
#include <element.h>
#include <entry.h>
#include <fileexporterbibtex.h>
#include "fileexporterpdf.h"

FileExporterPDF::FileExporterPDF(FileEmbedding fileEmbedding)
        : FileExporterToolchain(), m_fileEmbedding(fileEmbedding)
{
    m_laTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-pdf.tex");
    m_bibTeXFilename = tempDir.name() + QLatin1String("/bibtex-to-pdf.bib");
    m_outputFilename = tempDir.name() + QLatin1String("/bibtex-to-pdf.pdf");

    /// If there is not embedfile.sty file, disable embedding
    /// irrespective of user's wishes
    if (!kpsewhich("embedfile.sty")) m_fileEmbedding = NoFileEmbedding;

    reloadConfig();
}

FileExporterPDF::~FileExporterPDF()
{
    // nothing
}

void FileExporterPDF::reloadConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("kbibtexrc"));
    KConfigGroup configGroup(config, QLatin1String("FileExporterPDFPS"));
    m_babelLanguage = configGroup.readEntry(keyBabelLanguage, defaultBabelLanguage);
    m_bibliographyStyle = configGroup.readEntry(keyBibliographyStyle, defaultBibliographyStyle);

    KConfigGroup configGroupGeneral(config, QLatin1String("General"));
    m_paperSize = configGroupGeneral.readEntry(keyPaperSize, defaultPaperSize);
    m_font = configGroupGeneral.readEntry(keyFont, defaultFont);
}

bool FileExporterPDF::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    bool result = false;
    m_embeddedFileList.clear();
    if (m_fileEmbedding & EmbedBibTeXFile)
        m_embeddedFileList.append(QString("%1|%2|%3").arg("BibTeX source").arg(m_bibTeXFilename).arg(QLatin1String("bibtex-to-pdf.bib")));
    if (m_fileEmbedding & EmbedReferences)
        fillEmbeddedFileList(bibtexfile);

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("latex"));
        result = bibtexExporter->save(&output, bibtexfile, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generatePDF(iodevice, errorLog);

    return result;
}

bool FileExporterPDF::save(QIODevice *iodevice, const QSharedPointer<const Element> element, QStringList *errorLog)
{
    bool result = false;
    m_embeddedFileList.clear();
    //if (m_fileEmbedding & EmbedReferences)
    // FIXME need File object    fillEmbeddedFileList(element);

    QFile output(m_bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX *bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("latex"));
        result = bibtexExporter->save(&output, element, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = generatePDF(iodevice, errorLog);

    return result;
}

void FileExporterPDF::setDocumentSearchPaths(const QStringList &searchPaths)
{
    m_searchPaths = searchPaths;
}

bool FileExporterPDF::generatePDF(QIODevice *iodevice, QStringList *errorLog)
{
    QStringList cmdLines = QStringList() << QLatin1String("pdflatex -halt-on-error bibtex-to-pdf.tex") << QLatin1String("bibtex bibtex-to-pdf") << QLatin1String("pdflatex -halt-on-error bibtex-to-pdf.tex") << QLatin1String("pdflatex -halt-on-error bibtex-to-pdf.tex");

    return writeLatexFile(m_laTeXFilename) && runProcesses(cmdLines, errorLog) && writeFileToIODevice(m_outputFilename, iodevice, errorLog);
}

bool FileExporterPDF::writeLatexFile(const QString &filename)
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
        if (kpsewhich("hyperref.sty"))
            ts << "\\usepackage[pdfborder={0 0 0},pdfproducer={KBibTeX: http://home.gna.org/kbibtex/},pdftex]{hyperref}" << endl;
        else if (kpsewhich("url.sty"))
            ts << "\\usepackage{url}" << endl;
        if (m_bibliographyStyle.startsWith("apacite") && kpsewhich("apacite.sty"))
            ts << "\\usepackage[bibnewpage]{apacite}" << endl;
        if ((m_bibliographyStyle == QLatin1String("agsm") || m_bibliographyStyle == QLatin1String("dcu") || m_bibliographyStyle == QLatin1String("jmr") || m_bibliographyStyle == QLatin1String("jphysicsB") || m_bibliographyStyle == QLatin1String("kluwer") || m_bibliographyStyle == QLatin1String("nederlands") || m_bibliographyStyle == QLatin1String("dcu") || m_bibliographyStyle == QLatin1String("dcu")) && kpsewhich("harvard.sty") && kpsewhich("html.sty"))
            ts << "\\usepackage{html}" << endl << "\\usepackage[dcucite]{harvard}" << endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << endl;
        if (kpsewhich("embedfile.sty"))
            ts << "\\usepackage{embedfile}" << endl;
        if (kpsewhich("geometry.sty"))
            ts << "\\usepackage[paper=" << m_paperSize << (m_paperSize.length() <= 2 ? "paper" : "") << "]{geometry}" << endl;
        if (!m_font.isEmpty() && kpsewhich(m_font + QLatin1String(".sty")))
            ts << "\\usepackage{" << m_font << "}" << endl;
        ts << "\\bibliographystyle{" << m_bibliographyStyle << "}" << endl;
        ts << "\\begin{document}" << endl;

        if (!m_embeddedFileList.isEmpty())
            for (QStringList::ConstIterator it = m_embeddedFileList.constBegin(); it != m_embeddedFileList.constEnd(); ++it) {
                QStringList param = (*it).split("|");
                QFile file(param[1]);
                if (file.exists())
                    ts << "\\embedfile[desc={" << param[0] << "}";
                ts << ",filespec={" << param[2] << "}";
                if (param[2].endsWith(QLatin1String(".bib")))
                    ts << ",mimetype={text/x-bibtex}";
                else if (param[2].endsWith(QLatin1String(".pdf")))
                    ts << ",mimetype={application/pdf}";
                ts << "]{" << param[1] << "}" << endl;
            }

        ts << "\\nocite{*}" << endl;
        ts << "\\bibliography{bibtex-to-pdf}" << endl;
        ts << "\\end{document}" << endl;
        latexFile.close();
        return true;
    } else
        return false;
}

void FileExporterPDF::fillEmbeddedFileList(const File *bibtexfile)
{
    for (File::ConstIterator it = bibtexfile->constBegin(); it != bibtexfile->constEnd(); ++it)
        fillEmbeddedFileList(*it, bibtexfile);
}

void FileExporterPDF::fillEmbeddedFileList(const QSharedPointer<const Element> element, const File *bibtexfile)
{
    if (bibtexfile == NULL || !bibtexfile->hasProperty(File::Url)) {
        /// If no valid File was provided or File is not saved, do not append files
        return;
    }

    const QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (!entry.isNull()) {
        const QString id = entry->id();
        QList<KUrl> urlList = FileInfo::entryUrls(entry.data(), bibtexfile->property(File::Url).toUrl(), FileInfo::TestExistanceYes);
        foreach(const KUrl &url, urlList) {
            if (!url.isLocalFile()) continue;
            const QString filename = url.pathOrUrl();
            const QString basename = QFileInfo(filename).fileName();
            m_embeddedFileList.append(QString("%1|%2|%3").arg(id).arg(filename).arg(basename));
        }
    }
}
