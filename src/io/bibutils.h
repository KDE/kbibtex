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

#ifndef IO_BIBUTILS_H
#define IO_BIBUTILS_H

#include <QIODevice>

#include "kbibtexio_export.h"

/**
 * @author Thomas Fischer
 */
class KBIBTEXIO_EXPORT BibUtils
{
public:
    enum Format { MODS = 0, BibTeX = 1, BibLaTeX = 2, ISI = 5, RIS = 6, EndNote = 10, EndNoteXML = 11, ADS = 15, WordBib = 16, Copac = 17, Med = 18 };

    BibUtils::Format format() const;
    void setFormat(const BibUtils::Format &format);

    static bool available();

protected:
    explicit BibUtils();

    // TODO migrate to KJob or KCompositeJob
    bool convert(QIODevice &source, const BibUtils::Format &sourceFormat, QIODevice &destination, const BibUtils::Format &destinationFormat) const;

private:
    class Private;
    Private *const d;
};

#endif // IO_BIBUTILS_H
