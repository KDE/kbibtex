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

#include "fileexportertoolchain.h"

#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QHash>
#include <QTextStream>
#include <QProcess>
#include <QProcessEnvironment>
#include <QVector>

#include <KLocalizedString>

#include <Preferences>
#include "logging_io.h"

FileExporterToolchain::FileExporterToolchain(QObject *parent)
        : FileExporter(parent)
{
    tempDir.setAutoRemove(true);
}

bool FileExporterToolchain::runProcesses(const QStringList &progs, bool doEmitProcessOutput)
{
    bool result = true;
    int i = 0;

    emit progress(0, progs.size());
    for (QStringList::ConstIterator it = progs.constBegin(); result && it != progs.constEnd(); ++it) {
        QCoreApplication::instance()->processEvents();
        QStringList args = (*it).split(' ');
        QString cmd = args.first();
        args.erase(args.begin());
        result &= runProcess(cmd, args, doEmitProcessOutput);
        emit progress(i++, progs.size());
    }
    QCoreApplication::instance()->processEvents();
    return result;
}

bool FileExporterToolchain::runProcess(const QString &cmd, const QStringList &args, bool doEmitProcessOutput)
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

    qCInfo(LOG_KBIBTEX_IO) << "Running command" << fullCommandLine << "using working directory" << process.workingDirectory();
    process.start(cmd, args);

    connect(&process, &QProcess::readyReadStandardOutput, [this, &doEmitProcessOutput, &process] {
        QTextStream ts(process.readAllStandardOutput());
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            qCWarning(LOG_KBIBTEX_IO) << line;
            if (doEmitProcessOutput)
                emit processStandardOut(line);
        }
    });
    connect(&process, &QProcess::readyReadStandardError, [this, &doEmitProcessOutput, &process] {
        QTextStream ts(process.readAllStandardError());
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            qCDebug(LOG_KBIBTEX_IO) << line;
            if (doEmitProcessOutput)
                emit processStandardError(line);
        }
    });

    if (process.waitForStarted(3000)) {
        if (process.waitForFinished(30000)) {
            if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0)
                qCInfo(LOG_KBIBTEX_IO) << "Command" << fullCommandLine << "succeeded";
            else {
                qCWarning(LOG_KBIBTEX_IO) << "Command" << fullCommandLine << "failed with exit code" << process.exitCode() << ":" << process.errorString();
                return false;
            }
        } else {
            qCWarning(LOG_KBIBTEX_IO) << "Timeout waiting for command" << fullCommandLine << "failed:" << process.errorString();
            return false;
        }
    } else {
        qCWarning(LOG_KBIBTEX_IO) << "Starting command" << fullCommandLine << "failed:" << process.errorString();
        return false;
    }

    return true;
}

bool FileExporterToolchain::writeFileToIODevice(const QString &filename, QIODevice *device)
{
    bool result = false;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        static const qint64 buffersize = 0x10000;
        char buffer[buffersize];
        result = true;
        while (result) {
            const qint64 indata_size = file.read(buffer, buffersize);
            result &= indata_size >= 0;
            if (!result || indata_size == 0) break; //< on error or end of data

            qint64 remainingdata_size = buffersize;
            qint64 outdata_size = 0;
            int buffer_offset = 0;
            while (result && outdata_size < remainingdata_size) {
                buffer_offset += outdata_size;
                remainingdata_size -= outdata_size;
                outdata_size = device->write(buffer + buffer_offset, remainingdata_size);
                result &= outdata_size >= 0;
            }
        }

        file.close();
    }

    if (!result)
        qCWarning(LOG_KBIBTEX_IO) << "Writing to file" << filename << "failed";
    return result;
}

QString FileExporterToolchain::pageSizeToLaTeXName(const QPageSize::PageSizeId pageSizeId) const
{
    for (const auto &dbItem : Preferences::availablePageSizes)
        if (dbItem.first == pageSizeId)
            return dbItem.second;
    return QPageSize::name(pageSizeId).toLower(); ///< just a wild guess
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
