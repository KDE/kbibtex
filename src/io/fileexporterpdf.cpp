/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <KBibTeX>
#include <Preferences>
#include <Element>
#include <Entry>
#include "fileinfo.h"
#include "fileexporterbibtex.h"
#include "logging_io.h"

FileExporterPDF::FileExporterPDF(QObject *parent)
        : FileExporterToolchain(parent)
{
    m_fileBasename = QStringLiteral("bibtex-to-pdf");
    m_fileStem = tempDir.path() + QDir::separator() + m_fileBasename;

    setFileEmbedding(FileExporterPDF::FileEmbedding::BibTeXFileAndReferences);
}

FileExporterPDF::~FileExporterPDF()
{
    /// nothing
}

bool FileExporterPDF::save(QIODevice *iodevice, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;
    m_embeddedFileList.clear();
    if (m_fileEmbeddings.testFlag(FileEmbedding::BibTeXFile))
        m_embeddedFileList.append(QString(QStringLiteral("%1|%2|%3")).arg(QStringLiteral("BibTeX source"), m_fileStem + KBibTeX::extensionBibTeX, m_fileBasename + KBibTeX::extensionBibTeX));
    if (m_fileEmbeddings.testFlag(FileEmbedding::References))
        fillEmbeddedFileList(bibtexfile);

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, bibtexfile);
        output.close();
    }

    if (result)
        result = generatePDF(iodevice);

    return result;
}

bool FileExporterPDF::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
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
        result = bibtexExporter.save(&output, element, bibtexfile);
        output.close();
    }

    if (result)
        result = generatePDF(iodevice);

    return result;
}

void FileExporterPDF::setDocumentSearchPaths(const QStringList &searchPaths)
{
    m_searchPaths = searchPaths;
}

void FileExporterPDF::setFileEmbedding(const FileEmbeddings fileEmbedding) {
    /// If there is not embedfile.sty file, disable embedding
    /// irrespective of user's wishes
    if (!kpsewhich(QStringLiteral("embedfile.sty")))
        m_fileEmbeddings = FileEmbedding::None;
    else
        m_fileEmbeddings = fileEmbedding;
}

bool FileExporterPDF::generatePDF(QIODevice *iodevice)
{
    QStringList cmdLines {QStringLiteral("pdflatex -halt-on-error ") + m_fileStem + KBibTeX::extensionTeX, QStringLiteral("bibtex ") + m_fileStem + KBibTeX::extensionAux, QStringLiteral("pdflatex -halt-on-error ") + m_fileStem + KBibTeX::extensionTeX, QStringLiteral("pdflatex -halt-on-error ") + m_fileStem + KBibTeX::extensionTeX};

    return writeLatexFile(m_fileStem + KBibTeX::extensionTeX) && runProcesses(cmdLines) && writeFileToIODevice(m_fileStem + KBibTeX::extensionPDF, iodevice);
}

bool FileExporterPDF::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        ts.setCodec("UTF-8");
#if QT_VERSION >= 0x050e00
        ts << "\\documentclass{article}" << Qt::endl;
        ts << "\\usepackage[T1]{fontenc}" << Qt::endl;
        ts << "\\usepackage[utf8]{inputenc}" << Qt::endl;
#else // QT_VERSION < 0x050e00
        ts << "\\documentclass{article}" << endl;
        ts << "\\usepackage[T1]{fontenc}" << endl;
        ts << "\\usepackage[utf8]{inputenc}" << endl;
#endif // QT_VERSION >= 0x050e00
        if (kpsewhich(QStringLiteral("babel.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage[" << Preferences::instance().laTeXBabelLanguage() << "]{babel}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage[" << Preferences::instance().laTeXBabelLanguage() << "]{babel}" << endl;
#endif // QT_VERSION >= 0x050e00
        if (kpsewhich(QStringLiteral("hyperref.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage[pdfborder={0 0 0},pdfproducer={KBibTeX: https://userbase.kde.org/KBibTeX},pdftex]{hyperref}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage[pdfborder={0 0 0},pdfproducer={KBibTeX: https://userbase.kde.org/KBibTeX},pdftex]{hyperref}" << endl;
#endif // QT_VERSION >= 0x050e00
        else if (kpsewhich(QStringLiteral("url.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage{url}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage{url}" << endl;
#endif // QT_VERSION >= 0x050e00
        const QString bibliographyStyle = Preferences::instance().bibTeXBibliographyStyle();
        if (bibliographyStyle.startsWith(QStringLiteral("apacite")) && kpsewhich(QStringLiteral("apacite.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage[bibnewpage]{apacite}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage[bibnewpage]{apacite}" << endl;
#endif // QT_VERSION >= 0x050e00
        if ((bibliographyStyle == QStringLiteral("agsm") || bibliographyStyle == QStringLiteral("dcu") || bibliographyStyle == QStringLiteral("jmr") || bibliographyStyle == QStringLiteral("jphysicsB") || bibliographyStyle == QStringLiteral("kluwer") || bibliographyStyle == QStringLiteral("nederlands") || bibliographyStyle == QStringLiteral("dcu") || bibliographyStyle == QStringLiteral("dcu")) && kpsewhich(QStringLiteral("harvard.sty")) && kpsewhich(QStringLiteral("html.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage{html}" << Qt::endl << "\\usepackage[dcucite]{harvard}" << Qt::endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage{html}" << endl << "\\usepackage[dcucite]{harvard}" << endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << endl;
#endif // QT_VERSION >= 0x050e00
        if (kpsewhich(QStringLiteral("embedfile.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage{embedfile}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage{embedfile}" << endl;
#endif // QT_VERSION >= 0x050e00
        if (kpsewhich(QStringLiteral("geometry.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage[paper=" << pageSizeToLaTeXName(Preferences::instance().pageSize()) << "]{geometry}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage[paper=" << pageSizeToLaTeXName(Preferences::instance().pageSize()) << "]{geometry}" << endl;
#endif // QT_VERSION >= 0x050e00
#if QT_VERSION >= 0x050e00
        ts << "\\bibliographystyle{" << bibliographyStyle << "}" << Qt::endl;
        ts << "\\begin{document}" << Qt::endl;
#else // QT_VERSION < 0x050e00
        ts << "\\bibliographystyle{" << bibliographyStyle << "}" << endl;
        ts << "\\begin{document}" << endl;
#endif // QT_VERSION >= 0x050e00

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
#if QT_VERSION >= 0x050e00
                ts << "]{" << param[1] << "}" << Qt::endl;
#else // QT_VERSION < 0x050e00
                ts << "]{" << param[1] << "}" << endl;
#endif // QT_VERSION >= 0x050e00
            }

#if QT_VERSION >= 0x050e00
        ts << "\\nocite{*}" << Qt::endl;
        ts << QStringLiteral("\\bibliography{") << m_fileBasename << QStringLiteral("}") << Qt::endl;
        ts << "\\end{document}" << Qt::endl;
#else // QT_VERSION < 0x050e00
        ts << "\\nocite{*}" << endl;
        ts << QStringLiteral("\\bibliography{") << m_fileBasename << QStringLiteral("}") << endl;
        ts << "\\end{document}" << endl;
#endif // QT_VERSION >= 0x050e00
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
        const auto urlList = FileInfo::entryUrls(entry, bibtexfile->property(File::Url).toUrl(), FileInfo::TestExistence::Yes);
        for (const QUrl &url : urlList) {
            if (!url.isLocalFile()) continue;
            const QString filename = url.toLocalFile();
            const QString basename = QFileInfo(filename).fileName();
            m_embeddedFileList.append(QString(QStringLiteral("%1|%2|%3")).arg(title, filename, basename));
        }
    }
}
