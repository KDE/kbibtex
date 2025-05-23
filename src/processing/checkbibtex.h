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

#ifndef KBIBTEX_PROCESSING_CHECKBIBTEX_H
#define KBIBTEX_PROCESSING_CHECKBIBTEX_H

#include <QSharedPointer>

#ifdef HAVE_KF
#include "kbibtexprocessing_export.h"
#endif // HAVE_KF

class QWidget;

class Entry;
class Element;
class File;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROCESSING_EXPORT CheckBibTeX
{
public:
    enum class CheckBibTeXResult { NoProblem, BibTeXWarning, BibTeXError, InvalidData, FailedToCheck };

    static CheckBibTeXResult checkBibTeX(QSharedPointer<Element> &element, const File *file, QWidget *parent);
    static CheckBibTeXResult checkBibTeX(QSharedPointer<Entry> &entry, const File *file, QWidget *parent);
};

#endif // KBIBTEX_PROCESSING_CHECKBIBTEX_H
