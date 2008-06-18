#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include <QFile>

#include <fileimporterbibtex.h>
#include <fileimporterris.h>
#include <fileexporterbibtex.h>
#include <fileexporterpdf.h>
#include <fileexporterps.h>
#include <fileexporterris.h>
#include <fileexporterxml.h>

class KBibTeXIOTest
{
public:
    KBibTeXIOTest(int argc, char *argv[]);
    int run();

protected:
    QString m_inputFileName;
    QString m_outputFileName;
};

KBibTeXIOTest::KBibTeXIOTest(int argc, char *argv[])
        : m_inputFileName(argc >= 3 ? QString(argv[argc-2]) : QString::null), m_outputFileName(argc >= 3 ? QString(argv[argc-1]) : QString::null)
{
// nothing
}

int KBibTeXIOTest::run()
{
    KBibTeX::IO::FileImporter *importer = NULL;
    KBibTeX::IO::FileExporter *exporter = NULL;

    if (m_inputFileName == QString::null || !QFile(m_inputFileName).exists()) {
        qCritical("Input file does not exist or name is invalid");
        return 1;
    }
    if (m_outputFileName == QString::null) {
        qCritical("Input file name is invalid");
        return 1;
    }
    if (QFile(m_outputFileName).exists()) {
        qCritical("Output file already exists");
        return 2;
    }

    if (m_inputFileName.endsWith(".bib"))
        importer = new KBibTeX::IO::FileImporterBibTeX();
    else if (m_inputFileName.endsWith(".ris"))
        importer = new KBibTeX::IO::FileImporterRIS();
    else {
        qCritical("Input format not supported");
        return 3;
    }

    if (m_outputFileName.endsWith(".bib"))
        exporter = new KBibTeX::IO::FileExporterBibTeX();
    else if (m_outputFileName.endsWith(".pdf"))
        exporter = new KBibTeX::IO::FileExporterPDF();
    else if (m_outputFileName.endsWith(".ps"))
        exporter = new KBibTeX::IO::FileExporterPS();
    else if (m_outputFileName.endsWith(".ris"))
        exporter = new KBibTeX::IO::FileExporterRIS();
    else if (m_outputFileName.endsWith(".xml"))
        exporter = new KBibTeX::IO::FileExporterXML();
    else {
        delete importer;
        qCritical("Output format not supported");
        return 3;
    }

    QFile inputfile(m_inputFileName);
    inputfile.open(QIODevice::ReadOnly);
    KBibTeX::IO::File *bibtexfile = importer->load(&inputfile);
    inputfile.close();

    QFile outputfile(m_outputFileName);
    outputfile.open(QIODevice::WriteOnly);
    exporter->save(&outputfile, bibtexfile);
    outputfile.close();

    delete importer;
    delete exporter;
    delete bibtexfile;
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    KBibTeXIOTest test(argc, argv);
    return test.run();
}
