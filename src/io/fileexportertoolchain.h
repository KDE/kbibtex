/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_IO_FILEEXPORTERTOOLCHAIN_H
#define KBIBTEX_IO_FILEEXPORTERTOOLCHAIN_H

#include <QTemporaryDir>

#include <FileExporter>
#include <Preferences>

#ifdef HAVE_KF
#include "kbibtexio_export.h"
#endif // HAVE_KF

class QString;

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileExporterToolchain : public FileExporter
{
    Q_OBJECT

public:
    explicit FileExporterToolchain(QObject *parent);

    static bool kpsewhich(const QString &filename);

Q_SIGNALS:
    void processStandardOut(const QString);
    void processStandardError(const QString);

protected:
    QTemporaryDir tempDir;

    bool runProcesses(const QStringList &progs, bool doEmitProcessOutput = false);
    bool runProcess(const QString &cmd, const QStringList &args, bool doEmitProcessOutput = false);
    bool writeFileToIODevice(const QString &filename, QIODevice *device);

    QString pageSizeToLaTeXName(const Preferences::PageSize pageSize) const;
};

#endif // KBIBTEX_IO_FILEEXPORTERTOOLCHAIN_H
