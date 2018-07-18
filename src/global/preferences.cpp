/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "preferences.h"

#include <KLocalizedString>

const QString Preferences::groupColor = QStringLiteral("Color Labels");
const QString Preferences::keyColorCodes = QStringLiteral("colorCodes");
const QStringList Preferences::defaultColorCodes {QStringLiteral("#cc3300"), QStringLiteral("#0033ff"), QStringLiteral("#009966"), QStringLiteral("#f0d000")};
const QString Preferences::keyColorLabels = QStringLiteral("colorLabels");
// FIXME
// clazy warns: QString(const char*) being called [-Wclazy-qstring-uneeded-heap-allocations]
// ... but using QStringLiteral may break the translation process?
const QStringList Preferences::defaultColorLabels {I18N_NOOP("Important"), I18N_NOOP("Unread"), I18N_NOOP("Read"), I18N_NOOP("Watch")};

const QString Preferences::groupGeneral = QStringLiteral("General");
const QString Preferences::keyBackupScope = QStringLiteral("backupScope");
const Preferences::BackupScope Preferences::defaultBackupScope = LocalOnly;
const QString Preferences::keyNumberOfBackups = QStringLiteral("numberOfBackups");
const int Preferences::defaultNumberOfBackups = 5;

const QString Preferences::groupUserInterface = QStringLiteral("User Interface");
const QString Preferences::keyElementDoubleClickAction = QStringLiteral("elementDoubleClickAction");
const Preferences::ElementDoubleClickAction Preferences::defaultElementDoubleClickAction = ActionOpenEditor;

const QString Preferences::keyEncoding = QStringLiteral("encoding");
const QString Preferences::defaultEncoding = QStringLiteral("LaTeX");
const QString Preferences::keyStringDelimiter = QStringLiteral("stringDelimiter");
const QString Preferences::defaultStringDelimiter = QStringLiteral("{}");
const QString Preferences::keyQuoteComment = QStringLiteral("quoteComment");
const Preferences::QuoteComment Preferences::defaultQuoteComment = qcNone;
const QString Preferences::keyKeywordCasing = QStringLiteral("keywordCasing");
const KBibTeX::Casing Preferences::defaultKeywordCasing = KBibTeX::cLowerCase;
const QString Preferences::keyProtectCasing = QStringLiteral("protectCasing");
const Qt::CheckState Preferences::defaultProtectCasing = Qt::PartiallyChecked;
const QString Preferences::keyListSeparator = QStringLiteral("ListSeparator");
const QString Preferences::defaultListSeparator = QStringLiteral("; ");

/**
 * Preferences for Data objects
 */
const QString Preferences::keyPersonNameFormatting = QStringLiteral("personNameFormatting");
const QString Preferences::personNameFormatLastFirst = QStringLiteral("<%l><, %s><, %f>");
const QString Preferences::personNameFormatFirstLast = QStringLiteral("<%f ><%l>< %s>");
const QString Preferences::defaultPersonNameFormatting = personNameFormatLastFirst;
