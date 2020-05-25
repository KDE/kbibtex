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

#ifndef KBIBTEX_IO_FILEEXPORTERBIBTEXOUTPUT_H
#define KBIBTEX_IO_FILEEXPORTERBIBTEXOUTPUT_H

#include <QStringList>

#include <FileExporterToolchain>

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileExporterBibTeXOutput : public FileExporterToolchain
{
    Q_OBJECT

public:
    enum class OutputType {BibTeXLogFile, BibTeXBlockList};
    explicit FileExporterBibTeXOutput(OutputType outputType, QObject *parent);
    ~FileExporterBibTeXOutput() override;

    bool save(QIODevice *iodevice, const File *bibtexfile) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile) override;

private:
    OutputType m_outputType;
    QString m_fileBasename;
    QString m_fileStem;

    bool generateOutput();
    bool writeLatexFile(const QString &filename);
};

#endif // KBIBTEX_IO_FILEEXPORTERBIBTEXOUTPUT_H
