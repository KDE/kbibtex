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

#ifndef KBIBTEX_GLOBAL_PREFERENCES_H
#define KBIBTEX_GLOBAL_PREFERENCES_H

#include "kbibtex.h"

/**
 @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class Preferences {
public:
    static Preferences &instance();
    ~Preferences();

protected:
    Preferences();

public:

    /// *** Bibliography system, as of now either BibTeX or BibLaTeX

    /// Bibliography system: either BibTeX or BibLaTeX
    enum BibliographySystem { BibTeX = 0, BibLaTeX = 1 };

    /// Default bibliography system if nothing else is set or defined
    static const BibliographySystem defaultBibliographySystem;
    /// Retrieve current bibliography system
    BibliographySystem bibliographySystem();
    /// Set bibliography system
    /// @return true if the set bibliography system is differed from the previous value, false if both were the same
    bool setBibliographySystem(const BibliographySystem bibliographySystem);
    /// Map of supported bibliography systems, should be the same as in enum BibliographySystem
    static const QMap<BibliographySystem, QString> availableBibliographySystems();

    /// *** Name formatting like "Firstname Lastname", "Lastname, Firstname", or any other combination

    /// Predefined value for a person formatting, where last name comes before first name
    static const QString personNameFormatLastFirst;
    /// Predefined value for a person formatting, where first name comes before last name
    static const QString personNameFormatFirstLast;
    /// Default name formatting for a person if nothing else is set or defined
    static const QString defaultPersonNameFormatting;
    /// Retrieve current name formatting
    QString personNameFormatting();
    /// Set name formatting
    /// @return true if the set formatting is differed from the previous value, false if both were the same
    bool setPersonNameFormatting(const QString &personNameFormatting);


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

private:
    Q_DISABLE_COPY(Preferences)

    class Private;
    Private *const d;
};

#endif // KBIBTEX_GLOBAL_PREFERENCES_H
