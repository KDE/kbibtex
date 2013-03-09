/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer                             *
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

#ifndef KBIBTEX_PART_PART_H
#define KBIBTEX_PART_PART_H

#include <kparts/part.h>

#include <bibtexeditor.h>

class KBibTeXPart : public KParts::ReadWritePart
{
    Q_OBJECT

    friend class KBibTeXBrowserExtension;

public:
    KBibTeXPart(QWidget *parentWidget, QObject *parent, bool browserViewWanted);
    virtual ~KBibTeXPart();

    void setModified(bool modified);

protected:
    virtual bool openFile();
    virtual bool saveFile();

protected:
    void setupActions(bool BrowserViewWanted);
    void fitActionSettings();

    /*
      protected Q_SLOTS: // action slots
        void onSelectAll();
        void onUnselect();
        void onSetCoding( int Coding );
        void onSetEncoding( int Encoding );
        void onSetShowsNonprinting( bool on );
        void onSetResizeStyle( int Style );
        void onToggleOffsetColumn( bool on );
        void onToggleValueCharColumns( int VisibleColunms );
    */

    /*
      private Q_SLOTS:
        // used to catch changes in the bytearray widget
        void onSelectionChanged( bool HasSelection );
    */

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

private:
    class KBibTeXPartPrivate;
    KBibTeXPartPrivate *const d;
};

#endif // KBIBTEX_PART_PART_H
