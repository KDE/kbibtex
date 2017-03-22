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
#ifndef BIBTEXFILEEXPORTERRTF_H
#define BIBTEXFILEEXPORTERRTF_H

#include "fileexportertoolchain.h"

class QTextStream;

/**
@author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileExporterRTF : public FileExporterToolchain
{
    Q_OBJECT

public:
    FileExporterRTF(QObject *parent);
    ~FileExporterRTF() override;

    void reloadConfig() override;

    bool save(QIODevice *iodevice, const File *bibtexfile, QStringList *errorLog = nullptr) override;
    bool save(QIODevice *iodevice, const QSharedPointer<const Element> element, const File *bibtexfile, QStringList *errorLog = nullptr) override;

private:
    QString m_fileBasename;
    QString m_fileStem;
    QString m_babelLanguage;
    QString m_bibliographyStyle;
    QString m_paperSize;

    bool generateRTF(QIODevice *iodevice, QStringList *errorLog);
    bool writeLatexFile(const QString &filename);
};

#endif
