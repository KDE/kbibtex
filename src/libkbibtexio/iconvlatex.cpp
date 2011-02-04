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

#include <KDebug>

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
    char *outputBufferStart = outputBuffer;
    size_t inputBufferBytesLeft = inputByteArray.size();
    size_t ouputBufferBytesLeft = maxBufferSize;
    Encoder *laTeXEncoder = EncoderLaTeX::currentEncoderLaTeX();

    kDebug() << "inputBufferBytesLeft = " << inputBufferBytesLeft << "   ouputBufferBytesLeft = " << ouputBufferBytesLeft;
    while (iconv(d->iconvHandle, &inputBuffer, &inputBufferBytesLeft, &outputBuffer, &ouputBufferBytesLeft) == (size_t)(-1)) {
        kDebug() << "1 outputBuffer = " << outputBufferStart << "   inputBuffer = " << inputBuffer;
        QString remainingString = QString::fromUtf8(inputBuffer);
        kDebug() << "unicode = " << remainingString.at(0).unicode() << " remaining = " << remainingString.mid(1);
        QChar problematicChar = remainingString.at(0);
        remainingString = remainingString.mid(1);
        inputByteArray = remainingString.toUtf8();
        inputBuffer = inputByteArray.data();
        inputBufferBytesLeft = inputByteArray.size();
        kDebug() << "inputBufferBytesLeft = " << inputBufferBytesLeft << "   ouputBufferBytesLeft = " << ouputBufferBytesLeft;
        kDebug() << "2 outputBuffer = " << outputBufferStart << "   inputBuffer = " << inputBuffer;

        QString encodedProblem = laTeXEncoder->encode(problematicChar);
        kDebug() << "encodedProblem = " << encodedProblem;
        QByteArray encodedProblemByteArray = encodedProblem.toUtf8();
        qstrncpy(outputBuffer, encodedProblemByteArray.data(), ouputBufferBytesLeft);
        kDebug()<<"size = "<<encodedProblemByteArray.size();
        ouputBufferBytesLeft -= encodedProblemByteArray.size();
        outputBuffer += encodedProblemByteArray.size();
    }

    kDebug() << "3 outputBuffer = " << outputBufferStart << "   inputBuffer = " << inputBuffer;
    outputByteArray.resize(maxBufferSize - ouputBufferBytesLeft);
    return outputByteArray;
}
