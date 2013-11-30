/***************************************************************************
*   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_PART_BROWSEREXTENSION_H
#define KBIBTEX_PART_BROWSEREXTENSION_H

#include <kparts/browserextension.h>

class KBibTeXPart;


/**
 * @short Extension for better support for embedding in browsers
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBibTeXBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    explicit KBibTeXBrowserExtension(KBibTeXPart *part);

public: // KParts::BrowserExtension API
    virtual void saveState(QDataStream &stream);
    virtual void restoreState(QDataStream &stream);

public Q_SLOTS:
    /** copy text to clipboard */
//     void copy();

private Q_SLOTS:
    /** selection has changed */
//     void onSelectionChanged( bool );

protected:
    KBibTeXPart *part;
};

#endif // KBIBTEX_PART_BROWSEREXTENSION_H
