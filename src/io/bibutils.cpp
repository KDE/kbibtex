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

#include "bibutils.h"

#include <QProcess>
#include <QBuffer>
#include <QByteArray>
#include <QStandardPaths>

#include "logging_io.h"

class BibUtils::Private
{
public:
    BibUtils::Format format;

    Private(BibUtils *parent)
            : format(BibUtils::MODS)
    {
        Q_UNUSED(parent)
    }
};

BibUtils::BibUtils()
        : d(new BibUtils::Private(this))
{
    /// nothing
}

BibUtils::~BibUtils()
{
    delete d;
}

void BibUtils::setFormat(const BibUtils::Format format) {
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
        static const QStringList programs = QStringList() << QStringLiteral("bib2xml") << QStringLiteral("isi2xml") << QStringLiteral("ris2xml") << QStringLiteral("end2xml");
        state = avail;
        for (const QString &program : programs) {
            const QString fullPath = QStandardPaths::findExecutable(program);
            if (fullPath.isEmpty()) {
                state = unavail; ///< missing a single program is reason to assume that BibUtils is not correctly installed
                break;
            }
        }
        if (state == avail)
            qCDebug(LOG_KBIBTEX_IO) << "BibUtils found, using it to import/export certain types of bibliographies";
        else if (state == unavail)
            qCWarning(LOG_KBIBTEX_IO) << "No or only an incomplete installation of BibUtils found";
    }
    return state == avail;
}

bool BibUtils::convert(QIODevice &source, const BibUtils::Format sourceFormat, QIODevice &destination, const BibUtils::Format destinationFormat) const {
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
    QString utf8Argument = QStringLiteral("-un");

    /// Determine part of BibUtils program name that represents source format
    switch (sourceFormat) {
    case MODS: bibUtilsProgram = QStringLiteral("xml"); utf8Argument = QStringLiteral("-nb"); break;
    case BibTeX: bibUtilsProgram = QStringLiteral("bib"); break;
    case BibLaTeX: bibUtilsProgram = QStringLiteral("biblatex"); break;
    case ISI: bibUtilsProgram = QStringLiteral("isi"); break;
    case RIS: bibUtilsProgram = QStringLiteral("ris"); break;
    case EndNote: bibUtilsProgram = QStringLiteral("end"); break;
    case EndNoteXML: bibUtilsProgram = QStringLiteral("endx"); break;
    /// case ADS not supported by BibUtils
    case WordBib: bibUtilsProgram = QStringLiteral("wordbib"); break;
    case Copac: bibUtilsProgram = QStringLiteral("copac"); break;
    case Med: bibUtilsProgram = QStringLiteral("med"); break;
    default:
        qCWarning(LOG_KBIBTEX_IO) << "Unsupported BibUtils input format:" << sourceFormat;
        return false;
    }

    bibUtilsProgram.append(QStringLiteral("2"));

    /// Determine part of BibUtils program name that represents destination format
    switch (destinationFormat) {
    case MODS: bibUtilsProgram.append(QStringLiteral("xml")); break;
    case BibTeX: bibUtilsProgram.append(QStringLiteral("bib")); break;
    /// case BibLaTeX not supported by BibUtils
    case ISI: bibUtilsProgram.append(QStringLiteral("isi")); break;
    case RIS: bibUtilsProgram.append(QStringLiteral("ris")); break;
    case EndNote: bibUtilsProgram.append(QStringLiteral("end")); break;
    /// case EndNoteXML not supported by BibUtils
    case ADS: bibUtilsProgram.append(QStringLiteral("ads")); break;
    case WordBib: bibUtilsProgram.append(QStringLiteral("wordbib")); break;
    /// case Copac not supported by BibUtils
    /// case Med not supported by BibUtils
    default:
        qCWarning(LOG_KBIBTEX_IO) << "Unsupported BibUtils output format:" << destinationFormat;
        return false;
    }

    /// Test if required BibUtils program is available
    bibUtilsProgram = QStandardPaths::findExecutable(bibUtilsProgram);
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
    const QStringList arguments = QStringList() << QStringLiteral("-i") << QStringLiteral("utf8") << utf8Argument;
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
