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

#ifndef KBIBTEX_PART_PARTADAPTOR_H
#define KBIBTEX_PART_PARTADAPTOR_H

#include <QtDBus/QtDBus>

class KBibTeXPart;
class FileView;

/**
 * @author Shunsuke Shimizu <grafi@grafi.jp>
 */
class KBibTeXPartAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.gna.KBibTeX.Document")

public:
    explicit KBibTeXPartAdaptor(KBibTeXPart *part);
    ~KBibTeXPartAdaptor();

public slots:
    bool isReadWrite();
    bool documentSave();
    QList<int> selectedEntryIndexes();
    QString entryIndexesToText(const QList<int> &entryIndexes);
    QString entryIndexesToReferences(const QList<int> &entryIndexes);
    bool insertUrl(const QString &text, int entryIndex);
    QList<int> insertEntries(const QString &text, const QString &mimeType);
    QList<int> insertBibTeXEntries(const QString &text);

private:
    class KBibTeXPartAdaptorPrivate;
    KBibTeXPartAdaptor::KBibTeXPartAdaptorPrivate *d;
};

#endif // KBIBTEX_PART_PARTADAPTOR_H
