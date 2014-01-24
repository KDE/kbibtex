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
    BibUtils *p;

public:
    BibUtils::Format format;

    Private(BibUtils *parent)
            : p(parent), format(BibUtils::MODS)
    {
        // TODO
    }
};

BibUtils::BibUtils()
        : d(new BibUtils::Private(this))
{
    // TODO
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
    if (state == untested) {
        static const QStringList programs = QStringList() << QLatin1String("bib2xml") << QLatin1String("isi2xml") << QLatin1String("ris2xml") << QLatin1String("end2xml");
        state = avail;
        foreach(const QString &program, programs) {
            const QString fullPath = KStandardDirs::findExe(program);
            if (fullPath.isEmpty()) {
                state = unavail;
                break;
            }
        }
    }
    return state == avail;
}

bool BibUtils::convert(QIODevice &source, const BibUtils::Format &sourceFormat, QIODevice &destination, const BibUtils::Format &destinationFormat) const {
    QString bibUtilsProgram;

    if (sourceFormat != MODS && destinationFormat != MODS) {
        QBuffer buffer;
        bool result = convert(source, sourceFormat, buffer, BibUtils::MODS);
        if (result)
            result = convert(buffer, BibUtils::MODS, destination, destinationFormat);
        return result;
    }

    QString utf8Argument = QLatin1String("-un");

    switch (sourceFormat) {
    case MODS: bibUtilsProgram = QLatin1String("xml"); utf8Argument = QLatin1String("-nb"); break;
    case BibTeX: bibUtilsProgram = QLatin1String("bib"); break;
    case BibLaTeX: bibUtilsProgram = QLatin1String("biblatex"); break;
    case ISI: bibUtilsProgram = QLatin1String("isi"); break;
    case RIS: bibUtilsProgram = QLatin1String("ris"); break;
    case EndNote: bibUtilsProgram = QLatin1String("end"); break;
    default:
        kWarning() << "Unsupported BibUtils input format:" << sourceFormat;
        return false;
    }

    bibUtilsProgram.append(QLatin1String("2"));

    switch (destinationFormat) {
    case MODS: bibUtilsProgram.append(QLatin1String("xml")); break;
    case BibTeX: bibUtilsProgram.append(QLatin1String("bib")); break;
    case BibLaTeX: bibUtilsProgram.append(QLatin1String("biblatex")); break;
    case ISI: bibUtilsProgram.append(QLatin1String("isi")); break;
    case RIS: bibUtilsProgram.append(QLatin1String("end")); break;
    case EndNote: bibUtilsProgram.append(QLatin1String("end")); break;
    default:
        kWarning() << "Unsupported BibUtils output format:" << destinationFormat;
        return false;
    }

    bibUtilsProgram = KStandardDirs::findExe(bibUtilsProgram);
    kDebug() << "bibutilsProgram" << bibUtilsProgram;
    if (bibUtilsProgram.isEmpty())
        return false;

    kDebug() << "source.isOpen" << source.isOpen();
    kDebug() << "source.isReadable" << source.isReadable();
    kDebug() << "destination.isOpen" << destination.isOpen();
    kDebug() << "destination.isWritable" << destination.isWritable();
    if (!source.isReadable() && !source.open(QIODevice::ReadOnly))
        return false;
    if (!destination.isWritable() && !destination.open(QIODevice::WriteOnly)) {
        source.close();
        return false;
    }

    QProcess bibUtilsProcess;
    const QStringList arguments = QStringList() << QLatin1String("-i") << QLatin1String("utf8") << utf8Argument;
    kDebug() << "Running" << bibUtilsProgram << arguments.join(QLatin1String(" "));
    bibUtilsProcess.start(bibUtilsProgram, arguments);

    bool result = bibUtilsProcess.waitForStarted();
    kDebug() << "bibUtilsProcess.waitForStarted" << result;
    if (result) {
        bibUtilsProcess.write(source.readAll());
        bibUtilsProcess.closeWriteChannel();
        result = bibUtilsProcess.waitForFinished();
        kDebug() << "bibUtilsProcess.waitForFinished" << result;
        const QString errorText = QString(bibUtilsProcess.readAllStandardError());
        kDebug() << "errorText" << errorText.left(256);
        if (result && bibUtilsProcess.exitStatus() == QProcess::NormalExit) {
            const QByteArray stdOut = bibUtilsProcess.readAllStandardOutput();
            kDebug() << "stdOut.size" << stdOut.size();
            if (!stdOut.isEmpty()) {
                const QString stdOutText = QString(stdOut);
                kDebug() << "stdOutText" << stdOutText.left(256);
                const int amountWritten = destination.write(stdOut);
                result = amountWritten == stdOut.size();
            } else
                result = false;
        }
        else
            result = false;
    }

    /// In case it did not terminate earlier
    bibUtilsProcess.terminate();

    kDebug() << "result" << result;

    source.close();
    destination.close();
    return result;
}
