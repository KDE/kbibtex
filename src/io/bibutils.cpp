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

#include "bibutils.h"

#include <QProcess>
#include <QBuffer>
#include <QByteArray>

#include <KDebug>
#include <KStandardDirs>

class BibUtils::Private
{
private:
    // UNUSED BibUtils *p;

public:
    BibUtils::Format format;

    Private(BibUtils */* UNUSED parent*/)
        : /* UNUSED p(parent),*/ format(BibUtils::MODS)
    {
        /// nothing
    }
};

BibUtils::BibUtils()
        : d(new BibUtils::Private(this))
{
    /// nothing
}

void BibUtils::setFormat(const BibUtils::Format &format) {
    d->format = format;
}

BibUtils::Format BibUtils::format() const {
    return d->format;
}

bool BibUtils::available() {
    enum State {untested, avail, unavail};
    static State state = untested;
    /// Perform test only once, later rely on statically stored result
    if (state == untested) {
        /// Test a number of known BibUtils programs
        static const QStringList programs = QStringList() << QLatin1String("bib2xml") << QLatin1String("isi2xml") << QLatin1String("ris2xml") << QLatin1String("end2xml");
        state = avail;
        foreach(const QString &program, programs) {
            const QString fullPath = KStandardDirs::findExe(program);
            if (fullPath.isEmpty()) {
                state = unavail; ///< missing a single program is reason to assume that BibUtils is not correctly installed
                break;
            }
        }
    }
    return state == avail;
}

bool BibUtils::convert(QIODevice &source, const BibUtils::Format &sourceFormat, QIODevice &destination, const BibUtils::Format &destinationFormat) const {
    /// To proceed, either the source format or the destination format
    /// has to be MODS, otherwise ...
    if (sourceFormat != MODS && destinationFormat != MODS) {
        /// Add indirection: convert source format to MODS,
        /// then convert MODS data to destination format

        /// Intermediate buffer to hold MODS data
        QBuffer buffer;
        bool result = convert(source, sourceFormat, buffer, BibUtils::MODS);
        if (result)
            result = convert(buffer, BibUtils::MODS, destination, destinationFormat);
        return result;
    }

    QString bibUtilsProgram;
    QString utf8Argument = QLatin1String("-un");

    /// Determine part of BibUtils program name that represents source format
    switch (sourceFormat) {
    case MODS: bibUtilsProgram = QLatin1String("xml"); utf8Argument = QLatin1String("-nb"); break;
    case BibTeX: bibUtilsProgram = QLatin1String("bib"); break;
    case BibLaTeX: bibUtilsProgram = QLatin1String("biblatex"); break;
    case ISI: bibUtilsProgram = QLatin1String("isi"); break;
    case RIS: bibUtilsProgram = QLatin1String("ris"); break;
    case EndNote: bibUtilsProgram = QLatin1String("end"); break;
    case EndNoteXML: bibUtilsProgram = QLatin1String("endx"); break;
        /// case ADS not supported by BibUtils
    case WordBib: bibUtilsProgram = QLatin1String("wordbib"); break;
    case Copac: bibUtilsProgram = QLatin1String("copac"); break;
    case Med: bibUtilsProgram = QLatin1String("med"); break;
    default:
        kWarning() << "Unsupported BibUtils input format:" << sourceFormat;
        return false;
    }

    bibUtilsProgram.append(QLatin1String("2"));

    /// Determine part of BibUtils program name that represents destination format
    switch (destinationFormat) {
    case MODS: bibUtilsProgram.append(QLatin1String("xml")); break;
    case BibTeX: bibUtilsProgram.append(QLatin1String("bib")); break;
        /// case BibLaTeX not supported by BibUtils
    case ISI: bibUtilsProgram.append(QLatin1String("isi")); break;
    case RIS: bibUtilsProgram.append(QLatin1String("ris")); break;
    case EndNote: bibUtilsProgram.append(QLatin1String("end")); break;
        /// case EndNoteXML not supported by BibUtils
    case ADS: bibUtilsProgram.append(QLatin1String("ads")); break;
    case WordBib: bibUtilsProgram.append(QLatin1String("wordbib")); break;
        /// case Copac not supported by BibUtils
        /// case Med not supported by BibUtils
    default:
        kWarning() << "Unsupported BibUtils output format:" << destinationFormat;
        return false;
    }

    /// Test if required BibUtils program is available
    bibUtilsProgram = KStandardDirs::findExe(bibUtilsProgram);
    if (bibUtilsProgram.isEmpty())
        return false;

    /// Test if source device is readable
    if (!source.isReadable() && !source.open(QIODevice::ReadOnly))
        return false;
    /// Test if destination device is writable
    if (!destination.isWritable() && !destination.open(QIODevice::WriteOnly)) {
        source.close();
        return false;
    }

    QProcess bibUtilsProcess;
    const QStringList arguments = QStringList() << QLatin1String("-i") << QLatin1String("utf8") << utf8Argument;
    /// Start BibUtils program/process
    bibUtilsProcess.start(bibUtilsProgram, arguments);

    bool result = bibUtilsProcess.waitForStarted();
    if (result) {
        /// Write source data to process's stdin
        bibUtilsProcess.write(source.readAll());
        /// Close process's stdin start transformation
        bibUtilsProcess.closeWriteChannel();
        result = bibUtilsProcess.waitForFinished();

        /// If process run without problems ...
        if (result && bibUtilsProcess.exitStatus() == QProcess::NormalExit) {
            /// Read process's output, i.e. the transformed data
            const QByteArray stdOut = bibUtilsProcess.readAllStandardOutput();
            if (!stdOut.isEmpty()) {
                /// Write transformed data to destination device
                const int amountWritten = destination.write(stdOut);
                /// Check that the same amount of bytes is written
                /// as received from the BibUtils program
                result = amountWritten == stdOut.size();
            } else
                result = false;
        }
        else
            result = false;
    }

    /// In case it did not terminate earlier
    bibUtilsProcess.terminate();

    /// Close both source and destination device
    source.close();
    destination.close();

    return result;
}
