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

#ifndef KBIBTEX_GUI_ASSOCIATEDFILESUI_H
#define KBIBTEX_GUI_ASSOCIATEDFILESUI_H

#include <QWidget>

#include "entry.h"
#include "file.h"
#include "associatedfiles.h"

#include "kbibtexgui_export.h"

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT AssociatedFilesUI: public QWidget
{
    Q_OBJECT

public:
    static bool associateUrl(const QUrl &url, QSharedPointer<Entry> &entry, File *bibTeXfile, QWidget *parent);

    AssociatedFiles::RenameOperation renameOperation() const;
    AssociatedFiles::MoveCopyOperation moveCopyOperation() const;
    AssociatedFiles::PathType pathType() const;

protected:
    explicit AssociatedFilesUI(QSharedPointer<Entry> &entry, File *bibTeXfile, QWidget *parent);

private slots:
    void updateUIandPreview();

private:
    class Private;
    Private *const d;

    void setupForRemoteUrl(const QUrl &url, const QString &entryId);
    void setupForLocalFile(const QUrl &url, const QString &entryId);
};

#endif // KBIBTEX_GUI_ASSOCIATEDFILESUI_H
