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

#include "fileexportertoolchain.h"

#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QRegExp>
#include <QHash>
#include <QTextStream>
#include <QProcess>
#include <QProcessEnvironment>

#include <KLocalizedString>

const QString FileExporterToolchain::keyBabelLanguage = QStringLiteral("babelLanguage");
const QString FileExporterToolchain::defaultBabelLanguage = QStringLiteral("english");
const QString FileExporterToolchain::keyBibliographyStyle = QStringLiteral("bibliographyStyle");
const QString FileExporterToolchain::defaultBibliographyStyle = QStringLiteral("plain");

FileExporterToolchain::FileExporterToolchain(QObject *parent)
        : FileExporter(parent)
{
    tempDir.setAutoRemove(true);
}

bool FileExporterToolchain::runProcesses(const QStringList &progs, QStringList *errorLog)
{
    bool result = true;
    int i = 0;

    emit progress(0, progs.size());
    for (QStringList::ConstIterator it = progs.constBegin(); result && it != progs.constEnd(); ++it) {
        QCoreApplication::instance()->processEvents();
        QStringList args = (*it).split(' ');
        QString cmd = args.first();
        args.erase(args.begin());
        result &= runProcess(cmd, args, errorLog);
        emit progress(i++, progs.size());
    }
    QCoreApplication::instance()->processEvents();
    return result;
}

bool FileExporterToolchain::runProcess(const QString &cmd, const QStringList &args, QStringList *errorLog)
{
    QProcess process(this);
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    /// Avoid some paranoid security settings in BibTeX
    processEnvironment.insert(QStringLiteral("openout_any"), QStringLiteral("r"));
    /// Make applications use working directory as temporary directory
    processEnvironment.insert(QStringLiteral("TMPDIR"), tempDir.path());
    processEnvironment.insert(QStringLiteral("TEMPDIR"), tempDir.path());
    process.setProcessEnvironment(processEnvironment);
    process.setWorkingDirectory(tempDir.path());
    /// Assemble the full command line (program name + arguments)
    /// for use in log messages and debug output
    const QString fullCommandLine = cmd + QLatin1Char(' ') + args.join(QStringLiteral(" "));

    if (errorLog != nullptr)
        errorLog->append(i18n("Running command '%1' using working directory '%2'", fullCommandLine, process.workingDirectory()));
    process.start(cmd, args);

    if (errorLog != nullptr) {
        /// Redirect any standard output from process into errorLog
        connect(&process, &QProcess::readyReadStandardOutput, [errorLog, &process] {
            QByteArray stdout = process.readAllStandardOutput();
            QTextStream ts(&stdout);
            while (!ts.atEnd())
                errorLog->append(ts.readLine());
        });
        /// Redirect any standard error from process into errorLog
        connect(&process, &QProcess::readyReadStandardError, [errorLog, &process] {
            QByteArray stderr = process.readAllStandardError();
            QTextStream ts(&stderr);
            while (!ts.atEnd())
                errorLog->append(ts.readLine());
        });
    }

    bool result = process.waitForStarted(3000);
    if (!result) {
        if (errorLog != nullptr)
            errorLog->append(i18n("Starting command '%1' failed: %2", fullCommandLine, process.errorString()));
        return false;
    }

    if (process.waitForFinished(30000))
        result = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;

    if (errorLog != nullptr) {
        if (result)
            errorLog->append(i18n("Command '%1' succeeded", fullCommandLine));
        else
            errorLog->append(i18n("Command '%1' failed with exit code %2: %3", fullCommandLine, process.exitCode(), process.errorString()));
    }

    return result;
}

bool FileExporterToolchain::writeFileToIODevice(const QString &filename, QIODevice *device, QStringList *errorLog)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        bool result = true;
        static const qint64 buffersize = 0x10000;
        qint64 amount = 0;
        char buffer[buffersize];
        do {
            result = ((amount = file.read(buffer, buffersize)) > -1) && (device->write(buffer, amount) > -1);
        } while (result && amount > 0);

        file.close();

        if (errorLog != nullptr)
            errorLog->append(i18n("Writing to file '%1' succeeded", filename));
        return result;
    }

    if (errorLog != nullptr)
        errorLog->append(i18n("Writing to file '%1' failed", filename));
    return false;
}

bool FileExporterToolchain::kpsewhich(const QString &filename)
{
    static QHash<QString, bool> kpsewhichMap;
    if (kpsewhichMap.contains(filename))
        return kpsewhichMap.value(filename, false);

    bool result = false;
    QProcess kpsewhich;
    const QStringList param {filename};
    kpsewhich.start(QStringLiteral("kpsewhich"), param);
    if (kpsewhich.waitForStarted(3000) && kpsewhich.waitForFinished(30000)) {
        const QString standardOut = QString::fromUtf8(kpsewhich.readAllStandardOutput());
        result = kpsewhich.exitStatus() == QProcess::NormalExit && kpsewhich.exitCode() == 0 && standardOut.endsWith(QDir::separator() + filename + QChar('\n'));
        kpsewhichMap.insert(filename, result);
    }

    return result;
}
