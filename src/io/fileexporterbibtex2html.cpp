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

#include "fileexporterbibtex2html.h"

#include <QFile>
#include <QStandardPaths>

#include <KLocalizedString>

#include "fileexporterbibtex.h"
#include "logging_io.h"

class FileExporterBibTeX2HTML::FileExporterBibTeX2HTMLPrivate
{
private:
    FileExporterBibTeX2HTML *p;
public:
    QString bibTeXFilename;
    QString outputFilename;
    QString bibStyle;

    FileExporterBibTeX2HTMLPrivate(FileExporterBibTeX2HTML *parent, const QString &workingDir)
            : p(parent) {
        bibTeXFilename = QString(workingDir).append("/bibtex-to-html.bib");
        outputFilename = QString(workingDir).append("/bibtex-to-html.html");
        bibStyle = QStringLiteral("plain");
    }

    bool generateHTML(QIODevice *iodevice) {
        if (!checkBSTexists(iodevice)) return false;
        if (!checkBibTeX2HTMLexists(iodevice)) return false;

        /// bibtex2html automatically appends ".html" to output filenames
        QString outputFilenameNoEnding = outputFilename;
        outputFilenameNoEnding.remove(QStringLiteral(".html"));

        QStringList args;
        args << QStringLiteral("-s") << bibStyle; /// BibTeX style (plain, alpha, ...)
        args << QStringLiteral("-o") << outputFilenameNoEnding; /// redirect the output
        args << QStringLiteral("-nokeys"); /// do not print the BibTeX keys
        args << QStringLiteral("-nolinks"); /// do not print any web link
        args << QStringLiteral("-nodoc"); /// only produces the body of the HTML documents
        args << QStringLiteral("-nobibsource"); /// do not produce the BibTeX entries file
        args << QStringLiteral("-debug"); /// verbose mode (to find incorrect BibTeX entries)
        args << bibTeXFilename;

        bool result = p->runProcess(QStringLiteral("bibtex2html"), args) && p->writeFileToIODevice(outputFilename, iodevice);

        return result;
    }

    bool checkBibTeX2HTMLexists(QIODevice *iodevice) {
        if (!QStandardPaths::findExecutable(QStringLiteral("bibtex2html")).isEmpty())
            return true;

        QTextStream ts(iodevice);
        ts << QStringLiteral("<div style=\"color: red; background: white;\">");
        ts << i18n("The program <strong>bibtex2html</strong> is not available.");
#if QT_VERSION >= 0x050e00
        ts << QStringLiteral("</div>") << Qt::endl;
#else // QT_VERSION < 0x050e00
        ts << QStringLiteral("</div>") << endl;
#endif // QT_VERSION >= 0x050e00
        ts.flush();
        return false;
    }


    bool checkBSTexists(QIODevice *iodevice) {
        if (p->kpsewhich(bibStyle + ".bst"))
            return true;

        QTextStream ts(iodevice);
        ts << QStringLiteral("<div style=\"color: red; background: white;\">");
        ts << i18n("The BibTeX style <strong>%1</strong> is not available.", bibStyle);
#if QT_VERSION >= 0x050e00
        ts << QStringLiteral("</div>") << Qt::endl;
#else // QT_VERSION < 0x050e00
        ts << QStringLiteral("</div>") << endl;
#endif // QT_VERSION >= 0x050e00
        ts.flush();
        return false;
    }
};

FileExporterBibTeX2HTML::FileExporterBibTeX2HTML(QObject *parent)
        : FileExporterToolchain(parent), d(new FileExporterBibTeX2HTMLPrivate(this, tempDir.path()))
{
    /// nothing
}

FileExporterBibTeX2HTML::~FileExporterBibTeX2HTML()
{
    delete d;
}

bool FileExporterBibTeX2HTML::save(QIODevice *iodevice, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile output(d->bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, bibtexfile);
        output.close();
    }

    if (result)
        result = d->generateHTML(iodevice);

    return result;
}

bool FileExporterBibTeX2HTML::save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile)
{
    if (!iodevice->isWritable() && !iodevice->isWritable()) {
        qCWarning(LOG_KBIBTEX_IO) << "Output device not writable";
        return false;
    }

    bool result = false;

    QFile output(d->bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX bibtexExporter(this);
        bibtexExporter.setEncoding(QStringLiteral("latex"));
        result = bibtexExporter.save(&output, element, bibtexfile);
        output.close();
    }

    if (result)
        result = d->generateHTML(iodevice);

    return result;
}

void FileExporterBibTeX2HTML::setLaTeXBibliographyStyle(const QString &bibStyle)
{
    d->bibStyle = bibStyle;
}

QStringList FileExporterBibTeX2HTML::availableLaTeXBibliographyStyles()
{
    static QStringList listOfBibStyles;
    if (listOfBibStyles.isEmpty()) {
        static const QStringList stylesToTestFor {QStringLiteral("abbrv"), QStringLiteral("acm"), QStringLiteral("alpha"), QStringLiteral("apalike"), QStringLiteral("ieeetr"), QStringLiteral("plain"), QStringLiteral("siam"), QStringLiteral("unsrt")};
        for (const QString &bibStyle : stylesToTestFor)
            if (kpsewhich(bibStyle + ".bst"))
                listOfBibStyles.append(bibStyle);
    }
    return listOfBibStyles;
}
