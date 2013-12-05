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
*   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
***************************************************************************/

#ifndef KBIBTEX_PREFERENCES_H
#define KBIBTEX_PREFERENCES_H

#include <QString>
#include <QStringList>

#include <KLocale>

#include "kbibtexnamespace.h"

namespace Preferences
{
enum BackupScope { NoBackup, LocalOnly, BothLocalAndRemote };

static const QString groupColor = QLatin1String("Color Labels");
static const QString keyColorCodes = QLatin1String("colorCodes");
static const QStringList defaultColorCodes = QStringList() << QLatin1String("#cc3300") << QLatin1String("#0033ff") << QLatin1String("#009966") << QLatin1String("#f0d000");
static const QString keyColorLabels = QLatin1String("colorLabels");
static const QStringList defaultcolorLabels = QStringList() << I18N_NOOP("Important") << I18N_NOOP("Unread") << I18N_NOOP("Read") << I18N_NOOP("Watch");

static const QString groupGeneral = QLatin1String("General");
static const QString keyBackupScope = QLatin1String("backupScope");
static const BackupScope defaultBackupScope = LocalOnly;
static const QString keyNumberOfBackups = QLatin1String("numberOfBackups");
static const int defaultNumberOfBackups = 5;

static const QString groupUserInterface = QLatin1String("User Interface");
static const QString keyElementDoubleClickAction = QLatin1String("elementDoubleClickAction");
enum ElementDoubleClickAction { ActionOpenEditor = 0, ActionViewDocument = 1 };
static const ElementDoubleClickAction defaultElementDoubleClickAction = ActionOpenEditor;

/**
 * Preferences for File objects
 */
enum QuoteComment { qcNone = 0, qcCommand = 1, qcPercentSign = 2 };

const QString keyEncoding = QLatin1String("encoding");
const QString defaultEncoding = QLatin1String("LaTeX");
const QString keyStringDelimiter = QLatin1String("stringDelimiter");
const QString defaultStringDelimiter = QLatin1String("{}");
const QString keyQuoteComment = QLatin1String("quoteComment");
const QuoteComment defaultQuoteComment = qcNone;
const QString keyKeywordCasing = QLatin1String("keywordCasing");
const KBibTeX::Casing defaultKeywordCasing = KBibTeX::cLowerCase;
const QString keyProtectCasing = QLatin1String("protectCasing");
const bool defaultProtectCasing = true;

}

#endif // KBIBTEX_PREFERENCES_H
