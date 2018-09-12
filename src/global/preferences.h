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

#ifndef KBIBTEX_GLOBAL_PREFERENCES_H
#define KBIBTEX_GLOBAL_PREFERENCES_H

#include "kbibtex.h"

/**
 @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class Preferences {
public:

    /// Bibliography system: either BibTeX or BibLaTeX
    enum BibliographySystem { BibTeX = 0, BibLaTeX = 1 };

    /// Default bibliography system if nothing else is set or defined
    static const BibliographySystem defaultBibliographySystem;
    /// Current bibliography system as read from configuration file
    static BibliographySystem bibliographySystem();
    /// Update bibliography system in configuration file
    /// @return true if the set bibliography system differed from the previous value, false if both were the same
    static bool setBibliographySystem(const BibliographySystem bibliographySystem);
    /// Map of supported bibliography systems, should be the same as in enum BibliographySystem
    static const QMap<BibliographySystem, QString> availableBibliographySystems();

enum BackupScope { NoBackup, LocalOnly, BothLocalAndRemote };
enum ElementDoubleClickAction { ActionOpenEditor = 0, ActionViewDocument = 1 };
/**
 * Preferences for File objects
 */
enum QuoteComment { qcNone = 0, qcCommand = 1, qcPercentSign = 2 };

static const QString groupColor;
static const QString keyColorCodes;
static const QStringList defaultColorCodes;
static const QString keyColorLabels;
static const QStringList defaultColorLabels;

static const QString groupGeneral;
static const QString keyBackupScope;
static const BackupScope defaultBackupScope;
static const QString keyNumberOfBackups;
static const int defaultNumberOfBackups;

static const QString groupUserInterface;
static const QString keyElementDoubleClickAction;
static const ElementDoubleClickAction defaultElementDoubleClickAction;

static const QString keyEncoding;
static const QString defaultEncoding;
static const QString keyStringDelimiter;
static const QString defaultStringDelimiter;
static const QString keyQuoteComment;
static const QuoteComment defaultQuoteComment;
static const QString keyKeywordCasing;
static const KBibTeX::Casing defaultKeywordCasing;
static const QString keyProtectCasing;
static const Qt::CheckState defaultProtectCasing;
static const QString keyListSeparator;
static const QString defaultListSeparator;

static const QString keyPersonNameFormatting;
static const QString personNameFormatLastFirst;
static const QString personNameFormatFirstLast;
static const QString defaultPersonNameFormatting;
};

#endif // KBIBTEX_GLOBAL_PREFERENCES_H
