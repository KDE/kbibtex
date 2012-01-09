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

#include <KDebug>
#include <KLocale>

#include "fileexporterbibtex.h"
#include "fileexporterbibtex2html.h"

class FileExporterBibTeX2HTML::FileExporterBibTeX2HTMLPrivate
{
private:
    FileExporterBibTeX2HTML *p;
public:
    QString bibTeXFilename;
    QString outputFilename;
    QString bibStyle;

    FileExporterBibTeX2HTMLPrivate(FileExporterBibTeX2HTML *parent, QString workingDir)
            : p(parent) {
        bibTeXFilename = QString(workingDir).append("/bibtex-to-html.bib");
        outputFilename = QString(workingDir).append("/bibtex-to-html.html");
        bibStyle = QLatin1String("plain");
    }

    bool generateHTML(QIODevice* iodevice, QStringList *errorLog) {
        if (!checkBSTexists(iodevice)) return false;
        if (!checkBibTeX2HTMLexists(iodevice)) return false;

        /// bibtex2html automatically appends ".html" to output filenames
        QString outputFilenameNoEnding = outputFilename;
        outputFilenameNoEnding.replace(QLatin1String(".html"), QLatin1String(""));

        QStringList args;
        args << "-s" << bibStyle; /// BibTeX style (plain, alpha, ...)
        args << "-o" << outputFilenameNoEnding; /// redirect the output
        args << "-nokeys"; /// do not print the BibTeX keys
        args << "-nolinks"; /// do not print any web link
        args << "-nodoc"; /// only produces the body of the HTML documents
        args << "-nobibsource"; /// do not produce the BibTeX entries file
        args << "-debug"; /// verbose mode (to find incorrect BibTeX entries)
        args << bibTeXFilename;

        bool result = p->runProcess("bibtex2html", args, errorLog) && p->writeFileToIODevice(outputFilename, iodevice, errorLog);

        return result;
    }

    bool checkBibTeX2HTMLexists(QIODevice* iodevice) {
        if (p->which("bibtex2html"))
            return true;

        QTextStream ts(iodevice);
        ts << QLatin1String("<div style=\"color: red; background: white;\">");
        ts << i18n("The program <strong>bibtex2html</strong> is not available.");
        ts << QLatin1String("</div>") << endl;
        ts.flush();
        return false;
    }


    bool checkBSTexists(QIODevice* iodevice) {
        if (p->kpsewhich(bibStyle + ".bst"))
            return true;

        QTextStream ts(iodevice);
        ts << QLatin1String("<div style=\"color: red; background: white;\">");
        ts << i18n("The BibTeX style <strong>%1</strong> is not available.", bibStyle);
        ts << QLatin1String("</div>") << endl;
        ts.flush();
        return false;
    }
};

FileExporterBibTeX2HTML::FileExporterBibTeX2HTML()
        : FileExporterToolchain(), d(new FileExporterBibTeX2HTMLPrivate(this, tempDir.name()))
{
    // nothing
}

FileExporterBibTeX2HTML::~FileExporterBibTeX2HTML()
{
    delete d;
}

void FileExporterBibTeX2HTML::reloadConfig()
{
    // nothing
}

bool FileExporterBibTeX2HTML::save(QIODevice* iodevice, const File* bibtexfile, QStringList *errorLog)
{
    bool result = false;

    QFile output(d->bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX * bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, bibtexfile, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = d->generateHTML(iodevice, errorLog);

    return result;
}

bool FileExporterBibTeX2HTML::save(QIODevice* iodevice, const QSharedPointer<const Element> element, QStringList *errorLog)
{
    bool result = false;

    QFile output(d->bibTeXFilename);
    if (output.open(QIODevice::WriteOnly)) {
        FileExporterBibTeX * bibtexExporter = new FileExporterBibTeX();
        bibtexExporter->setEncoding(QLatin1String("utf-8"));
        result = bibtexExporter->save(&output, element, errorLog);
        output.close();
        delete bibtexExporter;
    }

    if (result)
        result = d->generateHTML(iodevice, errorLog);

    return result;
}

void FileExporterBibTeX2HTML::setLaTeXBibliographyStyle(const QString& bibStyle)
{
    d->bibStyle = bibStyle;
}
