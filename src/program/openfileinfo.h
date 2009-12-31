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

#ifndef KBIBTEX_PROGRAM_OPENFILEINFO_H
#define KBIBTEX_PROGRAM_OPENFILEINFO_H

#include <QList>
#include <QDateTime>

namespace KParts
{
class ReadWritePart;
}

namespace KBibTeX
{
namespace Program {

class OpenFileInfoManager;

class OpenFileInfo : protected QObject
{

public:
    static const QString propertyEncoding;
    static const QString mimetypeBibTeX;

    enum StatusFlag {
        Open = 0x1,
        RecentlyUsed = 0x2,
        Favorite = 0x4
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)

    ~OpenFileInfo();

    KParts::ReadWritePart* part(QWidget *parent);
    KUrl url() const;
    void setProperty(const QString &key, const QString &value);
    QString property(const QString &key) const;

    unsigned int counter();
    QString caption();
    QString fullCaption();

    StatusFlags flags() const;
    void setFlags(StatusFlags statusFlags);
    void clearFlags(StatusFlags statusFlags);

    QDateTime lastAccess() const;

    friend class OpenFileInfoManager;

protected:
    OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType, const KUrl &url);
    void setUrl(const KUrl& url);
    void setLastAccess(const QDateTime& dateTime);

private:
    class OpenFileInfoPrivate;
    OpenFileInfoPrivate *d;
};

class OpenFileInfoManager: public QObject
{
    Q_OBJECT
public:
    ~OpenFileInfoManager();

    static OpenFileInfoManager* getOpenFileInfoManager();

    OpenFileInfo *create(const QString &mimeType = QLatin1String("application/x-bibtex"), const KUrl & url = KUrl());
    OpenFileInfo *contains(const KUrl&url) const;
    OpenFileInfo *currentFile() const;
    void changeUrl(OpenFileInfo *openFileInfo, const KUrl & url);
    void close(OpenFileInfo *openFileInfo);
    void setCurrentFile(OpenFileInfo *openFileInfo);
    QList<OpenFileInfo*> filteredItems(OpenFileInfo::StatusFlags required, OpenFileInfo::StatusFlags forbidden = 0);


    friend class OpenFileInfo;

signals:
    void currentChanged(OpenFileInfo *);
    void listsChanged(OpenFileInfo::StatusFlags statusFlags);

protected:
    void flagsChangedInternal(OpenFileInfo::StatusFlags statusFlags);

private:
    OpenFileInfoManager();

    static OpenFileInfoManager *singletonOpenFileInfoManager;

    class OpenFileInfoManagerPrivate;
    OpenFileInfoManagerPrivate *d;

private slots:
    void deferredListsChanged();
};
}
}

#endif // KBIBTEX_PROGRAM_OPENFILEINFO_H
