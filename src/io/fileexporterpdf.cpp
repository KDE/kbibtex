/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "fileexporterpdf.h"

#include <QFile>
#include <QStringList>
#include <QUrl>
#include <QTextStream>
#include <QDir>

#include <KSharedConfig>
#include <KConfigGroup>

#include "fileinfo.h"
#include "element.h"
#include "entry.h"
#include "fileexporterbibtex.h"
#include "kbibtex.h"
#include "logging_io.h"

FileExporterPDF::FileExporterPDF(QObject *parent)
        : FileExporterToolchain(parent)
{
    m_fileBasename = QStringLiteral("bibtex-to-pdf");
    m_fileStem = tempDir.path() + QDir::separator() + m_fileBasename;

    setFileEmbedding(FileExporterPDF::EmbedBibTeXFileAndReferences);

    reloadConfig();
}

FileExporterPDF::~FileExporterPDF()
{
    /// nothing
}

void FileExporterPDF::reloadConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));
    KConfigGroup configGroup(config, QStringLiteral("FileExporterPDFPS"));
    m_babelLanguage = configGroup.readEntry(keyBabelLanguage, defaultBabelLanguage);
    m_bibliographyStyle = configGroup.readEntry(keyBibliographyStyle, defaultBibliographyStyle);

    KConfigGroup configGroupGeneral(config, QStringLiteral("General"));
    m_paperSize = configGroupGeneral.readEntry(keyPaperSize, defaultPaperSize);
    m_font = configGroupGeneral.readEntry(keyFont, defaultFont);
}

bool FileExporterPDF::save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;
    m_embeddedFileList.clear();
    if (m_fileEmbedding & EmbedBibTeXFile)
        m_embeddedFileList.append(QString(QStringLiteral("%1|%2|%3")).arg(QStringLiteral("BibTeX source"), m_fileStem + KBibTeX::extensionBibTeX, m_fileBasename + KBibTeX::extensionBibTeX));
    if (m_fileEmbedding & EmbedReferences)
        fillEmbeddedFileList(bibtexfile);

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, bibtexfile, errorLog);
        output.close();
    }

    if (result)
        result = generatePDF(iodevice, errorLog);

    if (errorLog != nullptr)
        qCDebug(LOG_KBIBTEX_IO) << "errorLog" << errorLog->join(QStringLiteral(";"));

    iodevice->close();
    return result;
}

bool FileExporterPDF::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog)
{
    if (!iodevice->isWritable() && !iodevice->open(QIODevice::WriteOnly)) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;
    m_embeddedFileList.clear();
    //if (m_fileEmbedding & EmbedReferences)
    // FIXME need File object    fillEmbeddedFileList(element);

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, element, bibtexfile, errorLog);
        output.close();
    }

    if (result)
        result = generatePDF(iodevice, errorLog);

    iodevice->close();
    return result;
}

void FileExporterPDF::setDocumentSearchPaths(const QStringList &searchPaths)
{
    m_searchPaths = searchPaths;
}

void FileExporterPDF::setFileEmbedding(FileEmbedding fileEmbedding) {
    /// If there is not embedfile.sty file, disable embedding
    /// irrespective of user's wishes
    if (!kpsewhich(QStringLiteral("embedfile.sty")))
        m_fileEmbedding = NoFileEmbedding;
    else
        m_fileEmbedding = fileEmbedding;
}

bool FileExporterPDF::generatePDF(QIODevice *iodevice, QStringList *errorLog)
{
    QStringList cmdLines {QStringLiteral("pdflatex -halt-on-error ") + m_fileStem + KBibTeX::extensionTeX, QStringLiteral("bibtex ") + m_fileStem + KBibTeX::extensionAux, QStringLiteral("pdflatex -halt-on-error ") + m_fileStem + KBibTeX::extensionTeX, QStringLiteral("pdflatex -halt-on-error ") + m_fileStem + KBibTeX::extensionTeX};

    return writeLatexFile(m_fileStem + KBibTeX::extensionTeX) && runProcesses(cmdLines, errorLog) && writeFileToIODevice(m_fileStem + KBibTeX::extensionPDF, iodevice, errorLog);
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
        if (kpsewhich(QStringLiteral("babel.sty")))
            ts << "\\usepackage[" << m_babelLanguage << "]{babel}" << endl;
        if (kpsewhich(QStringLiteral("hyperref.sty")))
            ts << "\\usepackage[pdfborder={0 0 0},pdfproducer={KBibTeX: https://userbase.kde.org/KBibTeX},pdftex]{hyperref}" << endl;
        else if (kpsewhich(QStringLiteral("url.sty")))
            ts << "\\usepackage{url}" << endl;
        if (m_bibliographyStyle.startsWith(QStringLiteral("apacite")) && kpsewhich(QStringLiteral("apacite.sty")))
            ts << "\\usepackage[bibnewpage]{apacite}" << endl;
        if ((m_bibliographyStyle == QStringLiteral("agsm") || m_bibliographyStyle == QStringLiteral("dcu") || m_bibliographyStyle == QStringLiteral("jmr") || m_bibliographyStyle == QStringLiteral("jphysicsB") || m_bibliographyStyle == QStringLiteral("kluwer") || m_bibliographyStyle == QStringLiteral("nederlands") || m_bibliographyStyle == QStringLiteral("dcu") || m_bibliographyStyle == QStringLiteral("dcu")) && kpsewhich(QStringLiteral("harvard.sty")) && kpsewhich(QStringLiteral("html.sty")))
            ts << "\\usepackage{html}" << endl << "\\usepackage[dcucite]{harvard}" << endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << endl;
        if (kpsewhich(QStringLiteral("embedfile.sty")))
            ts << "\\usepackage{embedfile}" << endl;
        if (kpsewhich(QStringLiteral("geometry.sty")))
            ts << "\\usepackage[paper=" << m_paperSize << (m_paperSize.length() <= 2 ? "paper" : "") << "]{geometry}" << endl;
        if (!m_font.isEmpty() && kpsewhich(m_font + QStringLiteral(".sty")))
            ts << "\\usepackage{" << m_font << "}" << endl;
        ts << "\\bibliographystyle{" << m_bibliographyStyle << "}" << endl;
        ts << "\\begin{document}" << endl;

        if (!m_embeddedFileList.isEmpty())
            for (const QString &embeddedFile : const_cast<const QStringList &>(m_embeddedFileList)) {
                const QStringList param = embeddedFile.split(QStringLiteral("|"));
                QFile file(param[1]);
                if (file.exists())
                    ts << "\\embedfile[desc={" << param[0] << "}";
                ts << ",filespec={" << param[2] << "}";
                if (param[2].endsWith(KBibTeX::extensionBibTeX))
                    ts << ",mimetype={text/x-bibtex}";
                else if (param[2].endsWith(KBibTeX::extensionPDF))
                    ts << ",mimetype={application/pdf}";
                ts << "]{" << param[1] << "}" << endl;
            }

        ts << "\\nocite{*}" << endl;
        ts << QStringLiteral("\\bibliography{") << m_fileBasename << QStringLiteral("}") << endl;
        ts << "\\end{document}" << endl;
        latexFile.close();
        return true;
    } else
        return false;
}

void FileExporterPDF::fillEmbeddedFileList(const File *bibtexfile)
{
    for (const auto &element : const_cast<const File &>(*bibtexfile))
        fillEmbeddedFileList(element, bibtexfile);
}

void FileExporterPDF::fillEmbeddedFileList(const QSharedPointer<const Element> element, const File *bibtexfile)
{
    if (bibtexfile == nullptr || !bibtexfile->hasProperty(File::Url)) {
        /// If no valid File was provided or File is not saved, do not append files
        return;
    }

    const QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
    if (!entry.isNull()) {
        const QString title = PlainTextValue::text(entry->value(Entry::ftTitle));
        const auto urlList = FileInfo::entryUrls(entry, bibtexfile->property(File::Url).toUrl(), FileInfo::TestExistenceYes);
        for (const QUrl &url : urlList) {
            if (!url.isLocalFile()) continue;
            const QString filename = url.toLocalFile();
            const QString basename = QFileInfo(filename).fileName();
            m_embeddedFileList.append(QString(QStringLiteral("%1|%2|%3")).arg(title, filename, basename));
        }
    }
}
