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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef KBIBTEX_IO_BIBUTILS_H
#define KBIBTEX_IO_BIBUTILS_H

#include <QIODevice>

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

/**
 * This class encapsulates calling the various binary programs of the BibUtils program set.
 * BibUtils is available at http://sourceforge.net/projects/bibutils/
 *
 * This class is inherited by @see FileImporterBibUtils and @see FileExporterBibUtils,
 * which make use of its protected functions.
 * Using this class directly should only happen to call its public static functions.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT BibUtils
{
public:
    BibUtils(const BibUtils &other) = delete;
    BibUtils &operator= (const BibUtils &other) = delete;
    ~BibUtils();

    enum Format { MODS = 0, BibTeX = 1, BibLaTeX = 2, ISI = 5, RIS = 6, EndNote = 10, EndNoteXML = 11, ADS = 15, WordBib = 16, Copac = 17, Med = 18 };

    BibUtils::Format format() const;
    void setFormat(const BibUtils::Format format);

    /**
     * Test if BibUtils is installed. This test checks if a number of known
     * BibUtils binaries are available (i.e. found in PATH). If any binary
     * is missing, it is assumed that BibUtils is not available. The test is
     * performed only once and the result cached for future calls to this function.
     * @return true if BibUtils is correctly installed, false otherwise
     */
    static bool available();

protected:
    explicit BibUtils();

    // TODO migrate to KJob or KCompositeJob
    bool convert(QIODevice &source, const BibUtils::Format sourceFormat, QIODevice &destination, const BibUtils::Format destinationFormat) const;

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_IO_BIBUTILS_H
