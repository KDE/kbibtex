/***************************************************************************
 *   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#include <../config.h>

#ifdef HAVE_BIBUTILS

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStringList>

#include "fileimporterbibutils.h"

/** this is required once due to the BibUtils library */
lists asis  = { 0, 0, NULL };
lists corps = { 0, 0, NULL };
char progname[] = "KBibTeX";

FileImporterBibUtils::FileImporterBibUtils(InputFormat inputFormat)
        : FileImporter(), m_workingDir(createTempDir()), m_inputFormat(inputFormat), m_bibTeXImporter(new FileImporterBibTeX(FALSE, "latin1"))
{
// nothing
}

FileImporterBibUtils::~FileImporterBibUtils()
{
    deleteTempDir(m_workingDir);
    delete m_bibTeXImporter;
}

File* FileImporterBibUtils::load(QIODevice *iodevice)
{
    if (!iodevice->isReadable()) {
        qDebug("iodevice is not readable");
        return NULL;
    }
    if (!iodevice->isOpen()) {
        qDebug("iodevice is not open");
        return NULL;
    }

    m_mutex.lock();

    File *result = NULL;
    m_cancelFlag = FALSE;
    QString inputFileName = QString(m_workingDir).append("/input.tmp");
    QString bibFileName = QString(m_workingDir).append("/input.bib");

    /** write temporary data to file to be read by bibutils */
    QFile inputFile(inputFileName);
    if (inputFile.open(QIODevice::WriteOnly)) {
        const qint64 bufferSize = 16384;
        char *buffer = new char[bufferSize];
        qint64 readBytes = 0;
        while ((readBytes = iodevice->read(buffer, bufferSize)) > 0) {
            if (inputFile.write(buffer, readBytes) != readBytes) {
                qCritical() << "Cannot write data to " << inputFileName << ", error: " << iodevice->errorString() << "/" << inputFile.errorString();
                readBytes = -1;
                break;
            }
        }
        inputFile.close();
        delete buffer;
        if (readBytes == -1) {
            qCritical() << "cannot read data from buffer, error: " << iodevice->errorString() << "/" << inputFile.errorString();
            m_mutex.unlock();
            return NULL;
        }
    } else {
        qCritical() << "cannot open " << inputFileName;
        m_mutex.unlock();
        return NULL;
    }

    param p;
    bibl b;
    bibl_init(&b);
    bibl_initparams(&p, (int)m_inputFormat, BIBL_BIBTEXOUT);
    p.verbose = 3;
    FILE *fp = fopen(inputFileName.toAscii(), "r");
    int err = BIBL_ERR_BADINPUT;
    if (fp) {
        char *filename = strdup(inputFileName.toAscii());
        err = bibl_read(&b, fp, filename, (int)m_inputFormat, &p);
        fclose(fp);
        free(filename);
    }
    if (err != BIBL_OK) {
        qCritical() << "bibutils error when reading data of type " << m_inputFormat << ": error code " << err;
        m_mutex.unlock();
        return NULL;
    }

    fp = fopen(bibFileName.toAscii(), "w");
    err = -1;
    if (fp != NULL) {
        err = bibl_write(&b, fp, BIBL_BIBTEXOUT, &p);
        fclose(fp);
    }
    bibl_free(&b);
    if (err != BIBL_OK) {
        qCritical() << "bibutils error when writing data of type " << BIBL_BIBTEXOUT << ": error code " << err;
        m_mutex.unlock();
        return NULL;
    }

    QFile bibFile(bibFileName);
    if (bibFile.open(QIODevice::ReadOnly)) {
        result = m_bibTeXImporter->load(&bibFile);
        bibFile.close();
    }

    m_mutex.unlock();
    return result;
}

bool FileImporterBibUtils::guessCanDecode(const QString & text)
{
    return guessInputFormat(text) != ifUndefined;
}

FileImporterBibUtils::InputFormat FileImporterBibUtils::guessInputFormat(const QString &text)
{
    if (text.indexOf("TY  - ") >= 0)
        return ifRIS;
    else if (text.indexOf("%A ") >= 0)
        return ifEndNote;
    else if (text.indexOf("FN ISI Export Format") >= 0)
        return ifISI;
    else
        return ifUndefined;
// TODO: Add more formats
}

void FileImporterBibUtils::cancel()
{
    m_bibTeXImporter->cancel();
    m_cancelFlag = TRUE;
}

QString FileImporterBibUtils::createTempDir()
{
    QString result = QString::null;
    QFile *devrandom = new QFile("/dev/random");

    if (devrandom->open(QIODevice::ReadOnly)) {
        quint32 randomNumber;
        if (devrandom->read((char*) & randomNumber, sizeof(randomNumber)) > 0) {
            randomNumber |= 0x10000000;
            result = QString("/tmp/bibtex-%1").arg(randomNumber, sizeof(randomNumber) * 2, 16);
            if (!QDir().mkdir(result))
                result = QString::null;
        }
        devrandom->close();
    }

    delete devrandom;

    return result;
}

void FileImporterBibUtils::deleteTempDir(const QString& directory)
{
    QDir dir = QDir(directory);
    QStringList subDirs = dir.entryList(QDir::Dirs);
    for (QStringList::Iterator it = subDirs.begin(); it != subDirs.end(); it++) {
        if ((QString::compare(*it, ".") != 0) && (QString::compare(*it, "..") != 0))
            deleteTempDir(*it);
    }
    QStringList allEntries = dir.entryList();
    for (QStringList::Iterator it = allEntries.begin(); it != allEntries.end(); it++)
        dir.remove(*it);

    QDir().rmdir(directory);
}

#endif // HAVE_BIBUTILS
