/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_IO_FILEEXPORTERPS_H
#define KBIBTEX_IO_FILEEXPORTERPS_H

#include <FileExporterToolchain>

#ifdef HAVE_KF
#include "kbibtexio_export.h"
#endif // HAVE_KF

#include "fileexporter_p.h"

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileExporterPS : public FileExporterToolchain
{
    Q_OBJECT

public:
    explicit FileExporterPS(QObject *parent);
    ~FileExporterPS() override;

    bool save(QIODevice *iodevice, const File *bibtexfile) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> &element, const File *bibtexfile) override;

private:
    QString m_fileBasename;
    QString m_fileStem;

    bool generatePS(QIODevice *iodevice);
    bool writeLatexFile(const QString &filename);
    bool beautifyPostscriptFile(const QString &filename, const QString &title);
};

#endif // KBIBTEX_IO_FILEEXPORTERPS_H
