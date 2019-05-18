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

#ifndef KBIBTEX_PROCESSING_BIBLIOGRAPHYSERVICE_H
#define KBIBTEX_PROCESSING_BIBLIOGRAPHYSERVICE_H

#include <QObject>

#ifdef HAVE_KF5
#include "kbibtexprocessing_export.h"
#endif // HAVE_KF5

/**
 * To make (or test for) KBibTeX the default bibliography editor,
 * this class offers some essential functions. To make the association work,
 * KBibTeX's .desktop files have to be placed where KDE (including kbuildsycoca5)
 * can find them.
 *
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROCESSING_EXPORT BibliographyService : public QObject
{
    Q_OBJECT

public:
    explicit BibliographyService(QWidget *parentWidget);
    ~BibliographyService() override;

    /**
     * Set KBibTeX as default editor for supported
     * bibliographic file formats.
     */
    void setKBibTeXasDefault();

    /**
     * Test if KBibTeX is default editor for supported
     * bibliographic file formats.
     * @return true if KBibTeX is default editor, else false
     */
    bool isKBibTeXdefault() const;

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_PROCESSING_BIBLIOGRAPHYSERVICE_H
