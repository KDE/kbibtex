/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_PROCESSING_LYX_H
#define KBIBTEX_PROCESSING_LYX_H

#include <QObject>
#include <qplatformdefs.h>

#ifdef HAVE_KF
#include "kbibtexprocessing_export.h"
#endif // HAVE_KF

namespace KParts
{
class ReadOnlyPart;
}

class QWidget;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXPROCESSING_EXPORT LyX : public QObject
{
    Q_OBJECT
public:
    LyX(KParts::ReadOnlyPart *part, QWidget *widget);
    ~LyX() override;

    void setReferences(const QStringList &references);

#ifdef QT_LSTAT
    static QString guessLyXPipeLocation();
#endif // QT_LSTAT

private Q_SLOTS:
    void sendReferenceToLyX();

private:
    class LyXPrivate;
    LyXPrivate *d;
};

#endif // KBIBTEX_PROCESSING_LYX_H
