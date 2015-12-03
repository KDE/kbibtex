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

#ifndef KBIBTEX_PROGRAM_OPENFILEINFOADAPTOR_H
#define KBIBTEX_PROGRAM_OPENFILEINFOADAPTOR_H

#include <QtDBus/QtDBus>

class OpenFileInfo;
class OpenFileInfoManager;
class MDIWidget;

/**
 * @author Shunsuke Shimizu <grafi@grafi.jp>
 */
class OpenFileInfoAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.gna.KBibTeX.FileManager.FileInfo")

public:
    explicit OpenFileInfoAdaptor(OpenFileInfo *p, OpenFileInfoManager *pm);

public slots:
    bool flags() const;
    QString url() const;
    bool isModified() const;
    bool save() const;
    void setFlags(uchar flags);
    int documentId();

private:
    OpenFileInfo *ofi;
    MDIWidget *mdi;
};

/**
 * @author Shunsuke Shimizu <grafi@grafi.jp>
 */
class OpenFileInfoManagerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.gna.KBibTeX.FileManager")

public:
    explicit OpenFileInfoManagerAdaptor(OpenFileInfoManager *p);
    static int openFileInfoToFileId(OpenFileInfo *ofi);
    OpenFileInfo *fileIdToOpenFileInfo(int fileId) const;

public slots:
    uchar FlagOpen() const;
    uchar FlagRecentlyUsed() const;
    uchar FlagFavorite() const;
    uchar FlagHasName() const;
    int currentFileId() const;
    void setCurrentFileId(int fileId);
    int open(const QString &url);
    bool close(int fileId);

private:
    OpenFileInfoManager *ofim;
};

#endif // KBIBTEX_PROGRAM_OPENFILEINFOADAPTOR_H
