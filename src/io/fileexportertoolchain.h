/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#ifndef BIBTEXFILEEXPORTERTOOLCHAIN_H
#define BIBTEXFILEEXPORTERTOOLCHAIN_H

#include <QProcess>

#include <QTemporaryDir>

#include "fileexporter.h"

class QString;
class QStringList;

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileExporterToolchain : public FileExporter
{
    Q_OBJECT

public:
    static const QString keyBabelLanguage;
    static const QString defaultBabelLanguage;

    static const QString keyBibliographyStyle;
    static const QString defaultBibliographyStyle;

    FileExporterToolchain(QObject *parent);

    virtual void reloadConfig() = 0;

    static bool kpsewhich(const QString &filename);

public slots:
    void cancel() override;

protected:
    QTemporaryDir tempDir;

    bool runProcesses(const QStringList &progs, QStringList *errorLog = nullptr);
    bool runProcess(const QString &cmd, const QStringList &args, QStringList *errorLog = nullptr);
    bool writeFileToIODevice(const QString &filename, QIODevice *device, QStringList *errorLog = nullptr);

private:
    QProcess *m_process;
    QStringList *m_errorLog;

private slots:
    void slotReadProcessStandardOutput();
    void slotReadProcessErrorOutput();

};

#endif
