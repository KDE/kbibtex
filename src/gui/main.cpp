#include <QDirModel>
#include <QApplication>

#include <file.h>
#include <fileimporterbibtex.h>

#include "bibtexfileview.h"
#include "bibtexfilemodel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KBibTeX::IO::File *bibtexFile = NULL;
    if (argc > 1) {
        KBibTeX::IO::FileImporterBibTeX *importer = new KBibTeX::IO::FileImporterBibTeX("latex", false);
        QFile inputfile(argv[argc-1]);
        inputfile.open(QIODevice::ReadOnly);
        bibtexFile = importer->load(&inputfile);
        inputfile.close();
        delete importer;
    }

    KBibTeX::GUI::Widgets::BibTeXFileView view;
    KBibTeX::GUI::Widgets::BibTeXFileModel model;
    model.setBibTeXFile(bibtexFile);

    view.setModel(&model);

    view.show();

    return app.exec();
}
