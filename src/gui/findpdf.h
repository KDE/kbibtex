/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#ifndef KBIBTEX_GUI_FINDPDF_H
#define KBIBTEX_GUI_FINDPDF_H

#include "kbibtexio_export.h"

#include <QObject>

class QWidget;

class KJob;

class File;
class Entry;

class KBIBTEXIO_EXPORT FindPDF : public QObject
{
    Q_OBJECT
public:
    FindPDF(QWidget *parent);
    QString findPDF(const Entry &entry, const File *file);

private slots:
    void finished(KJob *job);
    void citeseerXfinished(KJob *job);
    void forcedStop();

private:
    class FindPDFPrivate;
    FindPDFPrivate *d;
};

#endif // KBIBTEX_GUI_FINDPDF_H
