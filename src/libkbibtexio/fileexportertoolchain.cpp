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
#include <stdlib.h>

#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QRegExp>

#include "fileexportertoolchain.h"

static const QRegExp chompRegExp = QRegExp("[\\n\\r]+$");

FileExporterToolchain::FileExporterToolchain()
        : FileExporter(), m_errorLog(NULL)
{
    tempDir.setAutoRemove(true);
}

bool FileExporterToolchain::runProcesses(const QStringList &progs, QStringList *errorLog)
{
    bool result = TRUE;
    int i = 0;

    emit progress(0, progs.size());
    for (QStringList::ConstIterator it = progs.begin(); result && it != progs.end(); it++) {
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

bool FileExporterToolchain::runProcess(const QString &cmd,  const QStringList &args, QStringList *errorLog)
{
    bool result = false;

    m_process = new QProcess();
    QProcessEnvironment processEnvironment = QProcessEnvironment::systemEnvironment();
    /// avoid some paranoid security settings in BibTeX
    processEnvironment.insert("openout_any", "r");
    m_process->setProcessEnvironment(processEnvironment);
    m_process->setWorkingDirectory(tempDir.name());
    connect(m_process, SIGNAL(readyRead()), this, SLOT(slotReadProcessOutput()));

    m_process->start(cmd, args);
    m_errorLog = errorLog;

    if (m_process->waitForStarted(3000)) {
        if (m_process->waitForFinished(30000))
            result = m_process->exitStatus() == QProcess::NormalExit;
        else
            result = false;
    } else
        result = false;

    if (!result)
        errorLog->append(QString("Process '%1' failed.").arg(args.join(" ")));

    disconnect(m_process, SIGNAL(readyRead()), this, SLOT(slotReadProcessOutput()));
    delete(m_process);
    m_process = NULL;

    return result;
}

bool FileExporterToolchain::writeFileToIODevice(const QString &filename, QIODevice *device)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        bool result = true;
        qint64 buffersize = 0x10000;
        qint64 amount = 0;
        char* buffer = new char[ buffersize ];
        do {
            result = ((amount = file.read(buffer, buffersize)) > -1) && (device->write(buffer, amount) > -1);
        } while (result && amount > 0);

        file.close();
        delete[] buffer;

        return result;
    }

    return false;
}

void FileExporterToolchain::cancel()
{
    if (m_process != NULL) {
        qWarning("Canceling process");
        m_process->terminate();
        m_process->kill();
    }
}

void FileExporterToolchain::slotReadProcessOutput()
{
    if (m_process) {
        m_process->setReadChannel(QProcess::StandardOutput);
        while (m_process->canReadLine()) {
            QString line = m_process->readLine();
            if (m_errorLog != NULL)
                m_errorLog->append(line.replace(chompRegExp, ""));
        }
        m_process->setReadChannel(QProcess::StandardError);
        while (m_process->canReadLine()) {
            QString line = m_process->readLine();
            if (m_errorLog != NULL)
                m_errorLog->append(line.replace(chompRegExp, ""));
        }
    }
}

bool FileExporterToolchain::kpsewhich(const QString& filename)
{
    bool result = false;

    QProcess kpsewhich;
    QStringList param;
    param << filename;
    kpsewhich.start("kpsewhich", param);

    if (kpsewhich.waitForStarted(3000)) {
        if (kpsewhich.waitForFinished(30000))
            result = kpsewhich.exitStatus() == QProcess::NormalExit;
        else
            result = false;
    } else
        result = false;

    return result;
}

bool FileExporterToolchain::which(const QString& filename)
{
    QStringList paths = QString(getenv("PATH")).split(QLatin1String(":")); // FIXME: Most likely not portable?
    for (QStringList::Iterator it = paths.begin(); it != paths.end(); ++it) {
        QFileInfo fi(*it + "/" + filename);
        if (fi.exists() && fi.isExecutable()) return true;
    }

    return false;
}
