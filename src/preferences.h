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

#ifndef KBIBTEX_PREFERENCES_H
#define KBIBTEX_PREFERENCES_H

#include <QString>
#include <QStringList>

#include <KLocalizedString>

#include "kbibtex.h"

const QString keyListSeparator = QStringLiteral("ListSeparator");
namespace Preferences
{
enum BackupScope { NoBackup, LocalOnly, BothLocalAndRemote };

static const QString groupColor = QStringLiteral("Color Labels");
static const QString keyColorCodes = QStringLiteral("colorCodes");
static const QStringList defaultColorCodes = QStringList() << QStringLiteral("#cc3300") << QStringLiteral("#0033ff") << QStringLiteral("#009966") << QStringLiteral("#f0d000");
static const QString keyColorLabels = QStringLiteral("colorLabels");
static const QStringList defaultcolorLabels = QStringList() << I18N_NOOP("Important") << I18N_NOOP("Unread") << I18N_NOOP("Read") << I18N_NOOP("Watch");

static const QString groupGeneral = QStringLiteral("General");
static const QString keyBackupScope = QStringLiteral("backupScope");
static const BackupScope defaultBackupScope = LocalOnly;
static const QString keyNumberOfBackups = QStringLiteral("numberOfBackups");
static const int defaultNumberOfBackups = 5;

static const QString groupUserInterface = QStringLiteral("User Interface");
static const QString keyElementDoubleClickAction = QStringLiteral("elementDoubleClickAction");
enum ElementDoubleClickAction { ActionOpenEditor = 0, ActionViewDocument = 1 };
static const ElementDoubleClickAction defaultElementDoubleClickAction = ActionOpenEditor;

/**
 * Preferences for File objects
 */
enum QuoteComment { qcNone = 0, qcCommand = 1, qcPercentSign = 2 };

const QString keyEncoding = QStringLiteral("encoding");
const QString defaultEncoding = QStringLiteral("LaTeX");
const QString keyStringDelimiter = QStringLiteral("stringDelimiter");
const QString defaultStringDelimiter = QStringLiteral("{}");
const QString keyQuoteComment = QStringLiteral("quoteComment");
const QuoteComment defaultQuoteComment = qcNone;
const QString keyKeywordCasing = QStringLiteral("keywordCasing");
const KBibTeX::Casing defaultKeywordCasing = KBibTeX::cLowerCase;
const QString keyProtectCasing = QStringLiteral("protectCasing");
const bool defaultProtectCasing = true;
const QString keyListSeparator = QStringLiteral("ListSeparator");
const QString defaultListSeparator = QStringLiteral("; ");

/**
 * Preferences for Data objects
 */
const QString keyPersonNameFormatting = QStringLiteral("personNameFormatting");
const QString personNameFormatLastFirst = QStringLiteral("<%l><, %s><, %f>");
const QString personNameFormatFirstLast = QStringLiteral("<%f ><%l>< %s>");
const QString defaultPersonNameFormatting = personNameFormatLastFirst;


}

#endif // KBIBTEX_PREFERENCES_H
