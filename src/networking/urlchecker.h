/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef URLCHECKER_H
#define URLCHECKER_H

#include <QObject>

#include <File>

#ifdef HAVE_KF
#include "kbibtexnetworking_export.h"
#endif // HAVE_KF

class KBIBTEXNETWORKING_EXPORT UrlChecker : public QObject
{
    Q_OBJECT
public:
    enum class Status {UrlValid = 0, UnexpectedFileType, Error40X, NetworkError, UnknownError};

    explicit UrlChecker(QObject *parent = nullptr);
    ~UrlChecker();

public Q_SLOTS:
    void startChecking(const File &bibtexFile);

Q_SIGNALS:
    void urlChecked(QUrl url, UrlChecker::Status status, QString msg);
    void finished();

private:
    class Private;
    Private *const d;
};

#endif // URLCHECKER_H
