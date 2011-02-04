/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <iconv.h>

#include <QString>

#include <encoderlatex.h>
#include "iconvlatex.h"

static const int maxBufferSize = 16384;

class IConvLaTeX::IConvLaTeXPrivate
{
private:
    IConvLaTeX *p;

public:
    iconv_t iconvHandle;

    IConvLaTeXPrivate(IConvLaTeX *parent)
            : p(parent) {
        // nothing
    }
};

IConvLaTeX::IConvLaTeX(const QString &destEncoding)
        : d(new IConvLaTeXPrivate(this))
{
    d->iconvHandle = iconv_open(destEncoding.toAscii().data(), QLatin1String("utf-8").latin1());
}

IConvLaTeX::~IConvLaTeX()
{
    iconv_close(d->iconvHandle);
}

QByteArray IConvLaTeX::encode(const QString &input)
{
    QByteArray inputByteArray = input.toUtf8();
    char *inputBuffer = inputByteArray.data();
    QByteArray outputByteArray(maxBufferSize, '\0');
    char *outputBuffer = outputByteArray.data();
    size_t inputBufferBytesLeft = inputByteArray.size();
    size_t ouputBufferBytesLeft = maxBufferSize;
    Encoder *laTeXEncoder = EncoderLaTeX::currentEncoderLaTeX();

    while (iconv(d->iconvHandle, &inputBuffer, &inputBufferBytesLeft, &outputBuffer, &ouputBufferBytesLeft) == (size_t)(-1)) {
        /// split text into character where iconv stopped and remaining text
        QString remainingString = QString::fromUtf8(inputBuffer);
        QChar problematicChar = remainingString.at(0);
        remainingString = remainingString.mid(1);

        /// setup input buffer to continue with remaining text
        inputByteArray = remainingString.toUtf8();
        inputBuffer = inputByteArray.data();
        inputBufferBytesLeft = inputByteArray.size();

        /// encode problematic character in LaTeX encoding and append to output buffer
        QString encodedProblem = laTeXEncoder->encode(problematicChar);
        QByteArray encodedProblemByteArray = encodedProblem.toUtf8();
        qstrncpy(outputBuffer, encodedProblemByteArray.data(), ouputBufferBytesLeft);
        ouputBufferBytesLeft -= encodedProblemByteArray.size();
        outputBuffer += encodedProblemByteArray.size();
    }

    /// cut away unused space
    outputByteArray.resize(maxBufferSize - ouputBufferBytesLeft);

    return outputByteArray;
}
