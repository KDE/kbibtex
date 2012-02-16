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

#include <QObject>
#include <QList>
#include <QDateTime>
#include <QVariant>

#include <KService>
#include <KUrl>
#include <KMimeType>

namespace KParts
{
class ReadOnlyPart;
}

class OpenFileInfoManager;

class OpenFileInfo : public QObject
{
    Q_OBJECT

public:
    static const QString mimetypeBibTeX;
    static const QString mimetypeRIS;

    /**
     * Finds a KMimeType with the given url.
     * Tries to guess a file's mime type by its extension first,
     * but falls back to KMimeType's findByUrl if that does not work.
     * Background: If a HTTP or WebDAV server claims that a .bib file
     * is of mime type application/octet-stream, KMimeType::findByUrl
     * will keep that assessment instead of inspecting the file
     * extension.
     *
     * @see KMimeType::findByUrl
     * @param url Url to analyze
     * @return Guessed mime type
     */
    static KMimeType::Ptr mimeTypeForUrl(const KUrl &url);

    enum StatusFlag {
        Open = 0x1,
        RecentlyUsed = 0x2,
        Favorite = 0x4,
        HasName = 0x8
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)

    ~OpenFileInfo();

    KParts::ReadOnlyPart* part(QWidget *parent, KService::Ptr servicePtr = KService::Ptr());

    QString shortCaption() const;
    QString fullCaption() const;
    QString mimeType() const;
    KUrl url() const;
    bool isModified() const;
    bool save();

    /**
     * Close the current file. The user may interrupt the closing
     * if the file is modified and he/she presses "Cancel" when asked
     * to close the modified file.
     *
     * @return returns true if the closing was successful, otherwise false
     */
    bool close();

    StatusFlags flags() const;
    void setFlags(StatusFlags statusFlags);
    void addFlags(StatusFlags statusFlags);
    void removeFlags(StatusFlags statusFlags);

    QDateTime lastAccess() const;
    void setLastAccess(const QDateTime& dateTime = QDateTime::currentDateTime());

    KService::List listOfServices();
    KService::Ptr defaultService();
    KService::Ptr currentService();

    friend class OpenFileInfoManager;

signals:
    void flagsChanged(OpenFileInfo::StatusFlags statusFlags);

protected:
    OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const KUrl &url);
    OpenFileInfo(OpenFileInfoManager *openFileInfoManager, const QString &mimeType = OpenFileInfo::mimetypeBibTeX);
    void setUrl(const KUrl& url);

private:
    class OpenFileInfoPrivate;
    OpenFileInfoPrivate *d;
};

Q_DECLARE_METATYPE(OpenFileInfo*);


class OpenFileInfoManager: public QObject
{
    Q_OBJECT

public:
    typedef QVector<OpenFileInfo*> OpenFileInfoList;

    OpenFileInfoManager();
    ~OpenFileInfoManager();

    OpenFileInfo *createNew(const QString& mimeType = OpenFileInfo::mimetypeBibTeX);
    OpenFileInfo *open(const KUrl& url);
    OpenFileInfo *contains(const KUrl& url) const;
    OpenFileInfo *currentFile() const;
    bool changeUrl(OpenFileInfo *openFileInfo, const KUrl & url);
    bool close(OpenFileInfo *openFileInfo);
    void setCurrentFile(OpenFileInfo *openFileInfo, KService::Ptr servicePtr = KService::Ptr());
    OpenFileInfoList filteredItems(OpenFileInfo::StatusFlags required, OpenFileInfo::StatusFlags forbidden = 0);

    friend class OpenFileInfo;

signals:
    void currentChanged(OpenFileInfo *, KService::Ptr);
    void flagsChanged(OpenFileInfo::StatusFlags statusFlags);

private:
    class OpenFileInfoManagerPrivate;
    OpenFileInfoManagerPrivate *d;

private slots:
    void deferredListsChanged();
    void restorePreviouslyOpenedFiles();
};

#endif // KBIBTEX_PROGRAM_OPENFILEINFO_H
