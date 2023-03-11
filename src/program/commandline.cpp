/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2022-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QSet>
#include <QFile>
#include <QFileInfo>
#include <QTimer>

#include "kbibtex-version.h"
#include <File>
#include <FileImporter>
#include <FileExporter>
#include <FileExporterBibTeX>
#include <IdSuggestions>

int main(int argc, char *argv[])
{
    int exitCode = 0;
    QCoreApplication coreApp(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kbibtex-cli"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KBIBTEX_VERSION_STRING));

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    cmdLineParser.addVersionOption();
    QCommandLineOption outputFileCLO{{QStringLiteral("o"), QStringLiteral("output")}, QStringLiteral("Write output to file, stdout if not specified"), QStringLiteral("outputfilename")};
    cmdLineParser.addOption(outputFileCLO);
    QCommandLineOption outputformatCLI{{QStringLiteral("O"), QStringLiteral("output-format")}, QStringLiteral("Provide suggestion for output format"), QStringLiteral("format")};
    cmdLineParser.addOption(outputformatCLI);
    QCommandLineOption idSuggestionFormatStringCLO{{QStringLiteral("format-id")}, QStringLiteral("Reformat all entry ids using this format string"), QStringLiteral("formatstring")};
    cmdLineParser.addOption(idSuggestionFormatStringCLO);
    cmdLineParser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Read from this file"));

    cmdLineParser.process(coreApp);

    QVector<QString> arguments;
    arguments.reserve(cmdLineParser.positionalArguments().length());
    for (const QString &pa : cmdLineParser.positionalArguments())
        if (pa.length() > 0)
            arguments.append(pa);

    if (arguments.length() < 1) {
        std::cerr << "No file to read from specified. Use  --help  for instructions." << std::endl;
        coreApp.exit(exitCode = 1);
    } else if (arguments.length() > 1) {
        std::cerr << "More than one input file specified. Use  --help  for instructions." << std::endl;
        coreApp.exit(exitCode = 1);
    } else {
        const QFileInfo inputFileInfo{arguments.constFirst()};
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
                    if (cmdLineParser.isSet(idSuggestionFormatStringCLO)) {
                        /// If the user requested applying a certain format string to all entry ids, perform this change here.
                        QSet<QString> previousNewId; //< Track newly generated entry ids to detect duplicates
                        const QString formatString{cmdLineParser.value(idSuggestionFormatStringCLO)}; //< extract format string from command line argument
                        if (formatString.isEmpty()) {
                            std::cerr << "Got empty format string" << std::endl;
                            coreApp.exit(exitCode = 1);
                        } else {
                            std::cerr << "Using the following format string:" << std::endl;
                            for (const QString &fse : IdSuggestions::formatStrToHuman(formatString))
                                std::cerr << " * " << fse.toLocal8Bit().constData() << std::endl;
                            for (QSharedPointer<Element> &element : *file) {
                                /// For every element in the loaded bibliography file ...
                                /// Check if element is an entry (something that has an 'id')
                                QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
                                if (!entry.isNull()) {
                                    /// Generate new id based on entry and format string
                                    const QString newId{IdSuggestions::formatId(*(entry.data()), formatString)};
                                    if (newId.isEmpty()) {
                                        std::cerr << "New id generated from entry '" << entry->id().toLocal8Bit().constData() << "' and format string '" << formatString.toLocal8Bit().constData() << "' is empty." << std::endl;
                                        coreApp.exit(exitCode = 1);
                                        break;
                                    }
                                    if (previousNewId.contains(newId))
                                        std::cerr << "New entry id '" << newId.toLocal8Bit().constData() << "' generated from entry '" << entry->id().toLocal8Bit().constData() << "' and format string '" << formatString.toLocal8Bit().constData() << "' matches a previously generated id, but applying it anyway." << std::endl;
                                    else
                                        previousNewId.insert(newId);
                                    /// Apply newly-generated entry id
                                    entry->setId(newId);
                                }
                            }
                        }
                    }

                    if (exitCode == 0) {
                        if (cmdLineParser.isSet(outputFileCLO)) {
                            const QFileInfo outputFileInfo{cmdLineParser.value(outputFileCLO)};
                            QFile outputfile(outputFileInfo.filePath());
                            if (outputfile.open(QFile::WriteOnly)) {
                                QString exporterClassHint;
                                if (cmdLineParser.isSet(outputformatCLI)) {
                                    const QString cmpTo = cmdLineParser.value(outputformatCLI).toLower();
                                    for (const QString &exporterClass : FileExporter::exporterClasses(outputFileInfo))
                                        if (exporterClass.toLower().contains(cmpTo)) {
                                            exporterClassHint = exporterClass;
                                            std::cerr << "Choosing exporter " << exporterClassHint.toLocal8Bit().constData() << " based on --output-format=" << cmpTo.toLocal8Bit().constData() << std::endl;
                                            break;
                                        }
                                }
                                FileExporter *exporter = FileExporter::factory(outputFileInfo, exporterClassHint, &coreApp);
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
