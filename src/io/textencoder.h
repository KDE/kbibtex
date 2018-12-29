/***************************************************************************
 *   Copyright (C) 2004-2016 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_TEXTENCODER_H
#define KBIBTEX_TEXTENCODER_H

#ifdef HAVE_KF5
#include "kbibtexio_export.h"
#endif // HAVE_KF5

class QString;
class QByteArray;
class QStringList;
class QTextCodec;

/**
 * This class is a specialized wrapper around QTextCodec. It will try to encode
 * all characters not supported by the chosen encoding using the special
 * "LaTeX" encoding.
 * Example: When choosing encoding "iso8859-1" (aka Latin-1), you can encode
 * a-umlaut (a with two dots above), but not en-dash. Therefore, the en-dash
 * will be rewritten as "--" which is the LaTeX way to write this character.
 * On the other side, a-umlaut can be kept.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT TextEncoder
{
public:
    static QByteArray encode(const QString &input, const QString &destinationEncoding);
    static QByteArray encode(const QString &input, const QTextCodec *destinationCodec);

    static const QStringList encodings;

private:
    explicit TextEncoder();

};

#endif // KBIBTEX_TEXTENCODER_H
