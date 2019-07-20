/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include <KParts/Part>
#include <KParts/ReadWritePart>
#include <KAboutData>

#include <NotificationHub>
#include <file/PartWidget>

class KBibTeXPart : public KParts::ReadWritePart, private NotificationListener
{
    Q_OBJECT

    friend class KBibTeXBrowserExtension;

public:
    KBibTeXPart(QWidget *parentWidget, QObject *parent, const KAboutData &componentData);
    ~KBibTeXPart() override;

    void setModified(bool modified) override;

    void notificationEvent(int eventId) override;

protected:
    bool openFile() override;
    bool saveFile() override;

protected:
    void setupActions();

protected slots:
    bool documentSave();
    bool documentSaveAs();
    bool documentSaveCopyAs();
    void elementViewDocument();
    void elementFindPDF();
    void applyDefaultFormatString();

private slots:
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
