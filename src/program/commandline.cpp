/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <iostream>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

#include <File>
#include <FileImporter>
#include <FileExporter>
#include <FileExporterBibTeX>

int main(int argc, char *argv[])
{
    int exitCode = 0;
    QCoreApplication coreApp(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kbibtex-cli"));

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();
    QCommandLineOption outputFileCLO{{QStringLiteral("o"), QStringLiteral("output")}, QStringLiteral("Write output to file, stdout if not specified"), QStringLiteral("outputfilename")};
    cmdLineParser.addOption(outputFileCLO);
    cmdLineParser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Read from this file"));

    cmdLineParser.process(coreApp);

    if (cmdLineParser.positionalArguments().length() < 1) {
        std::cerr << "No file to read from specified. Use  --help  for instructions." << std::endl;
        coreApp.exit(exitCode = 1);
    } else if (cmdLineParser.positionalArguments().length() > 1) {
        std::cerr << "More than one input file specified. Use  --help  for instructions." << std::endl;
        coreApp.exit(exitCode = 1);
    } else {
        const QFileInfo inputFileInfo{cmdLineParser.positionalArguments().constFirst()};
        if (!inputFileInfo.exists() || inputFileInfo.size() <= 0 || !inputFileInfo.isFile() || !inputFileInfo.isReadable()) {
            std::cerr << "Argument is not an existing, readable, non-empty file: " << inputFileInfo.filePath().toLocal8Bit().constData() << std::endl;
            coreApp.exit(exitCode = 1);
        } else {
            QFile inputFile(inputFileInfo.filePath());
            if (inputFile.open(QFile::ReadOnly)) {
                FileImporter *importer = FileImporter::factory(inputFileInfo, &coreApp);
                File *file = importer->load(&inputFile);
                inputFile.close();

                if (file == nullptr) {
                    std::cerr << "Failed to load file: " << inputFileInfo.filePath().toLocal8Bit().constData() << std::endl;
                    coreApp.exit(exitCode = 1);
                } else {
                    if (exitCode == 0) {
                        if (cmdLineParser.isSet(outputFileCLO)) {
                            const QFileInfo outputFileInfo{cmdLineParser.value(outputFileCLO)};
                            QFile outputfile(outputFileInfo.filePath());
                            if (outputfile.open(QFile::WriteOnly)) {
                                FileExporter *exporter = FileExporter::factory(outputFileInfo, &coreApp);
                                const bool ok = exporter->save(&outputfile, file);
                                outputfile.close();
                                if (!ok) {
                                    std::cerr << "Failed to write to this file: " << outputFileInfo.filePath().toLocal8Bit().constData() << std::endl;
                                    coreApp.exit(exitCode = 1);
                                }
                            } else {
                                std::cerr << "Cannot write to this file: " << outputFileInfo.filePath().toLocal8Bit().constData() << std::endl;
                                coreApp.exit(exitCode = 1);
                            }
                        } else {
                            /// No output filename specified, so dump BibTeX code to stdout
                            FileExporter *exporter = new FileExporterBibTeX(&coreApp);
                            const QString output{exporter->toString(file)};
                            std::cout << output.toLocal8Bit().constData() << std::endl;
                        }
                    }

                    delete file;
                }
            } else {
                std::cerr << "Cannot read from file '" << inputFileInfo.filePath().toLocal8Bit().constData() << "'" << std::endl;
                coreApp.exit(exitCode = 1);
            }
        }
    }

    QTimer::singleShot(100, QCoreApplication::instance(), []() {
        QCoreApplication::instance()->quit();
    });

    coreApp.exit(exitCode);

    const int r = coreApp.exec();
    return qMax(r, exitCode);
}
