/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "fileexporterrtf.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QDir>

#include <KBibTeX>
#include <Preferences>
#include <Element>
#include "fileexporterbibtex.h"
#include "fileexporter_p.h"
#include "logging_io.h"

FileExporterRTF::FileExporterRTF(QObject *parent)
        : FileExporterToolchain(parent)
{
    m_fileBasename = QStringLiteral("bibtex-to-rtf");
    m_fileStem = tempDir.path() + QDir::separator() + m_fileBasename;
}

FileExporterRTF::~FileExporterRTF()
{
    /// nothing
}

bool FileExporterRTF::save(QIODevice *iodevice, const File *bibtexfile)
{
    check_if_bibtexfile_or_iodevice_invalid(bibtexfile, iodevice);

    bool result = false;

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, bibtexfile);
        output.close();
    }

    if (result)
        result = generateRTF(iodevice);

    return result;
}

bool FileExporterRTF::save(QIODevice *iodevice, const QSharedPointer<const Element> &element, const File *bibtexfile)
{
    check_if_iodevice_invalid(iodevice);

    bool result = false;

    QFile output(m_fileStem + KBibTeX::extensionBibTeX);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, element, bibtexfile);
        output.close();
    }

    if (result)
        result = generateRTF(iodevice);

    return result;
}

bool FileExporterRTF::generateRTF(QIODevice *iodevice)
{
    QStringList cmdLines {QStringLiteral("latex -halt-on-error bibtex-to-rtf.tex"), QStringLiteral("bibtex bibtex-to-rtf"), QStringLiteral("latex -halt-on-error bibtex-to-rtf.tex"), QString(QStringLiteral("latex2rtf -i %1 bibtex-to-rtf.tex")).arg(Preferences::instance().laTeXBabelLanguage())};

    return writeLatexFile(m_fileStem + KBibTeX::extensionTeX) && runProcesses(cmdLines) && writeFileToIODevice(m_fileStem + KBibTeX::extensionRTF, iodevice);
}

bool FileExporterRTF::writeLatexFile(const QString &filename)
{
    QFile latexFile(filename);
    if (latexFile.open(QIODevice::WriteOnly)) {
        QTextStream ts(&latexFile);
        // https://forum.qt.io/topic/135724/qt-6-replacement-for-qtextcodec
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        ts.setCodec("UTF-8");
#else
        ts.setEncoding(QStringConverter::Utf8);
#endif
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
        if (kpsewhich(QStringLiteral("url.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage{url}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage{url}" << endl;
#endif // QT_VERSION >= 0x050e00
        const QString bibliographyStyle = Preferences::instance().bibTeXBibliographyStyle();
        if (bibliographyStyle == QStringLiteral("dcu") && kpsewhich(QStringLiteral("harvard.sty")) && kpsewhich(QStringLiteral("html.sty")))
#if QT_VERSION >= 0x050e00
            ts << "\\usepackage{html}" << Qt::endl << "\\usepackage[dcucite]{harvard}" << Qt::endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << Qt::endl;
#else // QT_VERSION < 0x050e00
            ts << "\\usepackage{html}" << endl << "\\usepackage[dcucite]{harvard}" << endl << "\\renewcommand{\\harvardurl}{URL: \\url}" << endl;
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
        ts << "\\nocite{*}" << Qt::endl;
        ts << "\\bibliography{bibtex-to-rtf}" << Qt::endl;
        ts << "\\end{document}" << Qt::endl;
#else // QT_VERSION < 0x050e00
        ts << "\\bibliographystyle{" << bibliographyStyle << "}" << endl;
        ts << "\\begin{document}" << endl;
        ts << "\\nocite{*}" << endl;
        ts << "\\bibliography{bibtex-to-rtf}" << endl;
        ts << "\\end{document}" << endl;
#endif // QT_VERSION >= 0x050e00
        latexFile.close();
        return true;
    }

    return false;
}
