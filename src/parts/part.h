/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_PART_PART_H
#define KBIBTEX_PART_PART_H

#include <QObject>

#include <kparts_version.h>
#include <KParts/Part>
#include <KParts/ReadWritePart>

#include <NotificationHub>
#include <file/PartWidget>

#if KPARTS_VERSION >= 0x054D00 // >= 5.77.0
class KPluginMetaData;
#else // KPARTS_VERSION < 0x054D00 // < 5.77.0
class KAboutData;
#endif // KPARTS_VERSION >= 0x054D00 // >= 5.77.0

class KBibTeXPart : public KParts::ReadWritePart, private NotificationListener
{
    Q_OBJECT

    friend class KBibTeXBrowserExtension;

public:
    KBibTeXPart(QWidget *parentWidget, QObject *parent,
#if KPARTS_VERSION >= 0x054D00 // >= 5.77.0
                const KPluginMetaData &metaData
#else // KPARTS_VERSION < 0x054D00 // < 5.77.0
                const KAboutData &componentData
#endif // KPARTS_VERSION >= 0x054D00 // >= 5.77.0
                , const QVariantList &);
    ~KBibTeXPart() override;

    void setModified(bool modified) override;

    void notificationEvent(int eventId) override;

protected:
    bool openFile() override;
    bool saveFile() override;

protected:
    void setupActions();

protected Q_SLOTS:
    bool documentSave();
    bool documentSaveAs();
    bool documentSaveCopyAs();
    bool documentSaveSelection();
    void elementViewDocument();
    void elementFindPDF();
    void applyDefaultFormatString();

private Q_SLOTS:
    void newEntryTriggered();
    void newMacroTriggered();
    void newCommentTriggered();
    void newPreambleTriggered();
    void updateActions();
    void fileExternallyChange(const QString &path);

private:
    class KBibTeXPartPrivate;
    KBibTeXPartPrivate *const d;
};

#endif // KBIBTEX_PART_PART_H
