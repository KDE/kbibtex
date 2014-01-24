/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef IO_FILEIMPORTERBIBUTILS_H
#define IO_FILEIMPORTERBIBUTILS_H

#include "fileimporter.h"
#include "bibutils.h"

/**
 * @author Thomas Fischer
 */
class KBIBTEXIO_EXPORT FileImporterBibUtils : public FileImporter, public BibUtils
{
public:
    explicit FileImporterBibUtils();
    ~FileImporterBibUtils();

    virtual File *load(QIODevice *iodevice);

private:
    class Private;
    Private *const d;

};

#endif // IO_FILEIMPORTERBIBUTILS_H
