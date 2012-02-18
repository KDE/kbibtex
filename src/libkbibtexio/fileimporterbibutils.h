/***************************************************************************
 *   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#include <../config.h>

#ifdef HAVE_BIBUTILS
#ifndef BIBTEXFILEIMPORTBIBUTILS_H
#define BIBTEXFILEIMPORTBIBUTILS_H

#include <bibutils/bibutils.h>

#include "fileimporter.h"
#include "fileimporterbibtex.h"

/**
 @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
*/
class FileImporterBibUtils : public FileImporter
{
public:
    enum InputFormat {ifRIS = BIBL_RISIN, ifMedLine = BIBL_MEDLINEIN, ifISI = BIBL_ISIIN, ifEndNote = BIBL_ENDNOTEIN, ifCOPAC = BIBL_COPACIN, ifMODS = BIBL_MODSIN, ifUndefined = -9999};

    FileImporterBibUtils(InputFormat inputFormat);
    ~FileImporterBibUtils();

    File* load(QIODevice *iodevice);
    static bool guessCanDecode(const QString & text);
    static InputFormat guessInputFormat(const QString &text);

public slots:
    void cancel();

private:
    bool m_cancelFlag;
    QString m_workingDir;
    InputFormat m_inputFormat;
    FileImporterBibTeX *m_bibTeXImporter;

    QString createTempDir();
    void deleteTempDir(const QString& directory);

};

#endif // BIBTEXFILEIMPORTBIBUTILS_H
#endif // HAVE_BIBUTILS
