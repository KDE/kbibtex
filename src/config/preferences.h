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

/// This file has been automatically generated using the script 'preferences-generator.py'
/// based on configuration data from file 'preferences.json'. If there are any problems or
/// bugs, you need to fix those two files and re-generated both 'preferences.h' and
/// 'preferences.cpp'. Manual changes in this file will be overwritten the next time the
/// script will be run. You have been warned.

#ifndef KBIBTEX_CONFIG_PREFERENCES_H
#define KBIBTEX_CONFIG_PREFERENCES_H

#include <QPageSize>
#include <QColor>

#include <KBibTeX>

#ifdef HAVE_KF5
#include "kbibtexconfig_export.h"
#endif // HAVE_KF5

class KBIBTEXCONFIG_EXPORT Preferences {
public:
    static Preferences &instance();
    ~Preferences();

    enum BackupScope { NoBackup, LocalOnly, BothLocalAndRemote };
    enum BibliographySystem { BibTeX = 0, BibLaTeX = 1 };
    enum FileViewDoubleClickAction { ActionOpenEditor = 0, ActionViewDocument = 1 };
    enum QuoteComment { qcNone = 0, qcCommand = 1, qcPercentSign = 2 };


    /// *** BibliographySystem of type BibliographySystem ***

    static const BibliographySystem defaultBibliographySystem;
    static const QVector<QPair<Preferences::BibliographySystem, QString>> availableBibliographySystems;
    BibliographySystem bibliographySystem();
    bool setBibliographySystem(const BibliographySystem bibliographySystem);


    /// *** PersonNameFormat of type QString ***

    static const QString personNameFormatLastFirst;
    static const QString personNameFormatFirstLast;
    static const QString defaultPersonNameFormat;
    const QString &personNameFormat();
    bool setPersonNameFormat(const QString &personNameFormat);


    /// *** CopyReferenceCommand of type QString ***

    static const QString defaultCopyReferenceCommand;
    static const QStringList availableCopyReferenceCommands;
    const QString &copyReferenceCommand();
    bool setCopyReferenceCommand(const QString &copyReferenceCommand);


    /// *** PageSize of type QPageSize::PageSizeId ***

    static const QPageSize::PageSizeId defaultPageSize;
    static const QVector<QPair<QPageSize::PageSizeId, QString>> availablePageSizes;
    QPageSize::PageSizeId pageSize();
    bool setPageSize(const QPageSize::PageSizeId pageSize);


    /// *** BackupScope of type BackupScope ***

    static const BackupScope defaultBackupScope;
    static const QVector<QPair<Preferences::BackupScope, QString>> availableBackupScopes;
    BackupScope backupScope();
    bool setBackupScope(const BackupScope backupScope);


    /// *** NumberOfBackups of type int ***

    static const int defaultNumberOfBackups;
    int numberOfBackups();
    bool setNumberOfBackups(const int numberOfBackups);


    /// *** IdSuggestionFormatStrings of type QStringList ***

    static const QStringList defaultIdSuggestionFormatStrings;
    const QStringList &idSuggestionFormatStrings();
    bool setIdSuggestionFormatStrings(const QStringList &idSuggestionFormatStrings);


    /// *** ActiveIdSuggestionFormatString of type QString ***

    static const QString defaultActiveIdSuggestionFormatString;
    const QString &activeIdSuggestionFormatString();
    bool setActiveIdSuggestionFormatString(const QString &activeIdSuggestionFormatString);


    /// *** LyXUseAutomaticPipeDetection of type bool ***

    static const bool defaultLyXUseAutomaticPipeDetection;
    bool lyXUseAutomaticPipeDetection();
    bool setLyXUseAutomaticPipeDetection(const bool lyXUseAutomaticPipeDetection);


    /// *** LyXPipePath of type QString ***

    static const QString defaultLyXPipePath;
    const QString &lyXPipePath();
    bool setLyXPipePath(const QString &lyXPipePath);


    /// *** BibTeXEncoding of type QString ***

    static const QString defaultBibTeXEncoding;
    static const QStringList availableBibTeXEncodings;
    const QString &bibTeXEncoding();
    bool setBibTeXEncoding(const QString &bibTeXEncoding);


    /// *** BibTeXStringDelimiter of type QString ***

    static const QString defaultBibTeXStringDelimiter;
    static const QStringList availableBibTeXStringDelimiters;
    const QString &bibTeXStringDelimiter();
    bool setBibTeXStringDelimiter(const QString &bibTeXStringDelimiter);


    /// *** BibTeXQuoteComment of type QuoteComment ***

    static const QuoteComment defaultBibTeXQuoteComment;
    static const QVector<QPair<Preferences::QuoteComment, QString>> availableBibTeXQuoteComments;
    QuoteComment bibTeXQuoteComment();
    bool setBibTeXQuoteComment(const QuoteComment bibTeXQuoteComment);


    /// *** BibTeXKeywordCasing of type KBibTeX::Casing ***

    static const KBibTeX::Casing defaultBibTeXKeywordCasing;
    static const QVector<QPair<KBibTeX::Casing, QString>> availableBibTeXKeywordCasings;
    KBibTeX::Casing bibTeXKeywordCasing();
    bool setBibTeXKeywordCasing(const KBibTeX::Casing bibTeXKeywordCasing);


    /// *** BibTeXProtectCasing of type bool ***

    static const bool defaultBibTeXProtectCasing;
    bool bibTeXProtectCasing();
    bool setBibTeXProtectCasing(const bool bibTeXProtectCasing);


    /// *** BibTeXListSeparator of type QString ***

    static const QString defaultBibTeXListSeparator;
    static const QStringList availableBibTeXListSeparators;
    const QString &bibTeXListSeparator();
    bool setBibTeXListSeparator(const QString &bibTeXListSeparator);


    /// *** LaTeXBabelLanguage of type QString ***

    static const QString defaultLaTeXBabelLanguage;
    const QString &laTeXBabelLanguage();
    bool setLaTeXBabelLanguage(const QString &laTeXBabelLanguage);


    /// *** BibTeXBibliographyStyle of type QString ***

    static const QString defaultBibTeXBibliographyStyle;
    const QString &bibTeXBibliographyStyle();
    bool setBibTeXBibliographyStyle(const QString &bibTeXBibliographyStyle);


    /// *** FileViewDoubleClickAction of type FileViewDoubleClickAction ***

    static const FileViewDoubleClickAction defaultFileViewDoubleClickAction;
    static const QVector<QPair<Preferences::FileViewDoubleClickAction, QString>> availableFileViewDoubleClickActions;
    FileViewDoubleClickAction fileViewDoubleClickAction();
    bool setFileViewDoubleClickAction(const FileViewDoubleClickAction fileViewDoubleClickAction);


    /// *** ColorCodes of type QVector<QPair<QColor, QString>> ***

    static const QVector<QPair<QColor, QString>> defaultColorCodes;
    const QVector<QPair<QColor, QString>> &colorCodes();
    bool setColorCodes(const QVector<QPair<QColor, QString>> &colorCodes);

private:
    Q_DISABLE_COPY(Preferences)

    explicit Preferences();

    class Private;
    Private *const d;
};

#endif // KBIBTEX_CONFIG_PREFERENCES_H
