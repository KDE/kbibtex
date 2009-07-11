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

#include <QCoreApplication>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QDebug>

#include "fileexportertoolchain.h"

using namespace KBibTeX::IO;

FileExporterToolchain::FileExporterToolchain()
        : FileExporter(), m_waitCond(), m_waitCondMutex(), m_errorLog(NULL)
{
    workingDir = createTempDir();
}

FileExporterToolchain::~FileExporterToolchain()
{
    deleteTempDir(workingDir);
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
    bool result = FALSE;

    m_process = new QProcess();
    m_process->setWorkingDirectory(workingDir);
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcessExited(int, QProcess::ExitStatus)));
    connect(m_process, SIGNAL(readyRead()), this, SLOT(slotReadProcessOutput()));

    m_process->start(cmd, args);
    m_errorLog = errorLog;
    int counter = 0;

    if (m_process->waitForStarted(3000)) {
        QCoreApplication::instance()->processEvents();
        m_waitCondMutex.lock();
        while (m_process->state() == QProcess::Running) {
            m_waitCond.wait(&m_waitCondMutex, 250);
            QCoreApplication::instance()->processEvents();

            counter++;
            if (counter > 400)
                m_process->terminate();
        }
        m_waitCondMutex.unlock();
        result = m_process->exitStatus() == QProcess::NormalExit && counter < 400;
    } else
        result = false;

    if (!result)
        errorLog->append(QString("Process '%1' failed.").arg(args.join(" ")));

    disconnect(m_process, SIGNAL(readyRead()), this, SLOT(slotReadProcessOutput()));
    disconnect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcessExited(int, QProcess::ExitStatus)));
    delete(m_process);
    m_process = NULL;

    return result;
}

bool FileExporterToolchain::writeFileToIODevice(const QString &filename, QIODevice *device)
{
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        bool result = TRUE;
        qint64 buffersize = 0x10000;
        qint64 amount = 0;
        char* buffer = new char[ buffersize ];
        do {
            result = ((amount = file.read(buffer, buffersize)) > -1) && (device->write(buffer, amount) > -1);
        } while (result && amount > 0);

        file.close();
        delete[] buffer;
        return result;
    } else
        return FALSE;
}

QString FileExporterToolchain::createTempDir()
{
    QString result = QString::null;
    QFile *devrandom = new QFile("/dev/random");

    if (devrandom->open(QIODevice::ReadOnly)) {
        quint32 randomNumber;
        if (devrandom->read((char*) & randomNumber, sizeof(randomNumber)) > 0) {
            randomNumber |= 0x10000000;
            result = QString("/tmp/bibtex-%1").arg(randomNumber, sizeof(randomNumber) * 2, 16);
            if (!QDir().mkdir(result))
                result = QString::null;
        }
        devrandom->close();
    }

    delete devrandom;

    return result;
}

void FileExporterToolchain::deleteTempDir(const QString& directory)
{
    QDir dir = QDir(directory);
    QStringList subDirs = dir.entryList(QDir::Dirs);
    for (QStringList::Iterator it = subDirs.begin(); it != subDirs.end(); it++) {
        if ((QString::compare(*it, ".") != 0) && (QString::compare(*it, "..") != 0))
            deleteTempDir(*it);
    }
    QStringList allEntries = dir.entryList();
    for (QStringList::Iterator it = allEntries.begin(); it != allEntries.end(); it++)
        dir.remove(*it);

    QDir().rmdir(directory);
}

void FileExporterToolchain::slotProcessExited(int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/)
{
    m_waitCond.wakeAll();
}

void FileExporterToolchain::cancel()
{
    if (m_process != NULL) {
        qWarning("Canceling process");
        m_process->terminate();
        m_process->kill();
        m_waitCond.wakeAll();
    }
}

void FileExporterToolchain::slotReadProcessOutput()
{
    if (m_process) {
        m_process->setReadChannel(QProcess::StandardOutput);
        while (m_process->canReadLine()) {
            QString line = m_process->readLine();
            if (m_errorLog != NULL)
                m_errorLog->append(line);
        }
        m_process->setReadChannel(QProcess::StandardError);
        while (m_process->canReadLine()) {
            QString line = m_process->readLine();
            if (m_errorLog != NULL)
                m_errorLog->append(line);
        }
    }
}

bool FileExporterToolchain::kpsewhich(const QString& filename)
{
    bool result = FALSE;
    int counter = 0;

    QWaitCondition waitCond;
    QMutex waitCondMutex;
    QProcess kpsewhich;
    QStringList param;
    param << filename;
    kpsewhich.start("kpsewhich", param);

    if (kpsewhich.waitForStarted(3000)) {
        waitCondMutex.lock();
        while (kpsewhich.state() == QProcess::Running) {
            waitCond.wait(&waitCondMutex, 250);
            QCoreApplication::instance()->processEvents();

            counter++;
            if (counter > 50)
                kpsewhich.terminate();
        }
        waitCondMutex.unlock();
        result = kpsewhich.exitStatus() == QProcess::NormalExit && counter < 50;
    } else
        result = false;

    return result;
}

// #include "fileexportertoolchain.moc"
