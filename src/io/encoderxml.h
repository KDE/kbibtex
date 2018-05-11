/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_IO_ENCODERXML_H
#define KBIBTEX_IO_ENCODERXML_H

#include "kbibtexio_export.h"

#include "encoder.h"

class QString;

/**
 * Base class for that convert between different textual representations
 * for non-ASCII characters, specialized for XML.
 * Example for a character to convert is &auml;.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT EncoderXML : public Encoder
{
public:
    ~EncoderXML() override;

    QString decode(const QString &text) const override;
    QString encode(const QString &text, const TargetEncoding targetEncoding) const override;

    static const EncoderXML &instance();

protected:
    EncoderXML();

private:
    Q_DISABLE_COPY(EncoderXML)

    class EncoderXMLPrivate;
    EncoderXMLPrivate *const d;
};

#endif // KBIBTEX_IO_ENCODERXML_H
