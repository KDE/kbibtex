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

#ifndef KBIBTEX_ICONVLATEX_H
#define KBIBTEX_ICONVLATEX_H

#include "kbibtexio_export.h"

class QStringList;

/**
 * This class is a specialized wrapper around iconv. It will try to encode
 * all characters not supported by the chosen encoding using the special
 * "LaTeX" encoding.
 * Example: When choosing encoding "iso8859-1" (aka Latin-1), you can encode
 * a-umlaut (a with two dots above), but not en-dash. Therefore, the en-dash
 * will be rewritten as "--" which is the LaTeX way to write this character.
 * On the other side, a-umlaut can be kept.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT IConvLaTeX
{
public:
    IConvLaTeX(const QString &destEncoding);
    ~IConvLaTeX();

    QByteArray encode(const QString &input);

    static const QStringList encodings();

private:
    class IConvLaTeXPrivate;
    IConvLaTeXPrivate *d;

    static QStringList encodingList;
};

#endif // KBIBTEX_ICONVLATEX_H
