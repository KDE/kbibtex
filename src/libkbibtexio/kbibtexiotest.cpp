#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include <QFile>

#include <fileimporterbibtex.h>
#include <fileexporterbibtex.h>

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

    KBibTeX::IO::FileImporterBibTeX importer;
    KBibTeX::IO::FileExporterBibTeX exporter;

    QFile inputfile(m_inputFileName);
    inputfile.open(QIODevice::ReadOnly);
    KBibTeX::IO::File *bibtexfile = importer.load(&inputfile);
    inputfile.close();

    QFile outputfile(m_outputFileName);
    outputfile.open(QIODevice::WriteOnly);
    exporter.save(&outputfile, bibtexfile);
    outputfile.close();

    delete bibtexfile;
    return 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    KBibTeXIOTest test(argc, argv);
    return test.run();
}
