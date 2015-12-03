/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                      2015 by Shunsuke Shimizu <grafi@grafi.jp>          *
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

#include "partadaptor.h"

#include <KDebug>

#include "part.h"
#include "partwidget.h"
#include "fileoperation.h"

/**
 * @author Shunsuke Shimizu <grafi@grafi.jp>
 */
class KBibTeXPartAdaptor::KBibTeXPartAdaptorPrivate
{
private:
    // UNUSED KBibTeXPartAdaptor *parent;

public:
    KBibTeXPart *part;
    PartWidget *widget;
    FileOperation *fileOpr;

    KBibTeXPartAdaptorPrivate(KBibTeXPart *pa, KBibTeXPartAdaptor */* UNUSED p*/)
            : part(pa), widget(qobject_cast<PartWidget *>(part->widget())), fileOpr(new FileOperation(widget->fileView())) {
        /// nothing
    }
};

KBibTeXPartAdaptor::KBibTeXPartAdaptor(KBibTeXPart *part)
        : QDBusAbstractAdaptor(part), d(new KBibTeXPartAdaptorPrivate(part, this))
{
    QString objectPath = QLatin1String("/KBibTeX/Documents/") + QString::number(d->widget->documentId());
    bool registerObjectResult = QDBusConnection::sessionBus().registerObject(objectPath, part);
    if (!registerObjectResult)
        kWarning() << "Failed to register this application to the DBus session bus";
}

KBibTeXPartAdaptor::~KBibTeXPartAdaptor()
{
    delete d;
}


bool KBibTeXPartAdaptor::isReadWrite()
{
    return d->part->isReadWrite();
}

bool KBibTeXPartAdaptor::documentSave()
{
    return d->part->documentSave();
}

QList<int> KBibTeXPartAdaptor::selectedEntryIndexes()
{
    return d->fileOpr->selectedEntryIndexes();
}

QString KBibTeXPartAdaptor::entryIndexesToText(const QList<int> &entryIndexes)
{
    return d->fileOpr->entryIndexesToText(entryIndexes);
}

QString KBibTeXPartAdaptor::entryIndexesToReferences(const QList<int> &entryIndexes)
{
    return d->fileOpr->entryIndexesToReferences(entryIndexes);
}

/**
 * Makes an attempt to insert the passed text as an URL to the given
 * element. May fail for various reasons, such as the text not being
 * a valid URL or the element being invalid.
 */
bool KBibTeXPartAdaptor::insertUrl(const QString &text, int entryIndex)
{
    return d->fileOpr->insertUrl(text, entryIndex);
}

/**
 * Parse a given text fragment for bibliographic entries
 * and insert them into the currently open bibliography file.
 * If no file is open, a new one will be created.
 *
 * @param text text fragement to be parsed for bibliographic entries
 * @param mimeType mime type specifying how the bibliographic entries are described
 * @return the indexes of inserted entries
 */
QList<int> KBibTeXPartAdaptor::insertEntries(const QString &text, const QString &mimeType)
{
    return d->fileOpr->insertEntries(text, mimeType);
}

/**
 * Assumption: user dropped a piece of BibTeX code,
 * use BibTeX importer to generate representation from plain text
 */
QList<int> KBibTeXPartAdaptor::insertBibTeXEntries(const QString &text)
{
    return d->fileOpr->insertBibTeXEntries(text);
}
