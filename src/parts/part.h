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

#ifndef KBIBTEX_PART_PART_H
#define KBIBTEX_PART_PART_H

#include <QObject>

#include <KParts/Part>
#include <KParts/ReadWritePart>
#include <KAboutData>

#include "notificationhub.h"
#include "partwidget.h"

class KBibTeXPart : public KParts::ReadWritePart, private NotificationListener
{
    Q_OBJECT

    friend class KBibTeXBrowserExtension;

public:
    KBibTeXPart(QWidget *parentWidget, QObject *parent, const QVariantList& args);
    virtual ~KBibTeXPart();

    void setModified(bool modified);

    virtual void notificationEvent(int eventId);

protected:
    virtual bool openFile();
    virtual bool saveFile();

protected:
    void setupActions();

protected slots:
    bool documentSave();
    bool documentSaveAs();
    bool documentSaveCopyAs();
    void elementViewDocument();
    void elementViewDocumentMenu(QObject *);
    void elementFindPDF();
    void applyDefaultFormatString();

private slots:
    void newElementTriggered(int event);
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
