/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

    enum class BackupScope { None, LocalOnly, BothLocalAndRemote };
    enum class BibliographySystem { BibTeX, BibLaTeX };
    enum class FileViewDoubleClickAction { OpenEditor, ViewDocument };
    enum class QuoteComment { None, Command, PercentSign };


    /// *** BibliographySystem of type BibliographySystem ***

    static const BibliographySystem defaultBibliographySystem;
    static const QVector<QPair<Preferences::BibliographySystem, QString>> availableBibliographySystems;
    BibliographySystem bibliographySystem();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibliographySystem(const BibliographySystem bibliographySystem);
#endif // HAVE_KF5


    /// *** PersonNameFormat of type QString ***

    static const QString personNameFormatLastFirst;
    static const QString personNameFormatFirstLast;
    static const QString defaultPersonNameFormat;
    const QString &personNameFormat();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setPersonNameFormat(const QString &personNameFormat);
#endif // HAVE_KF5


    /// *** CopyReferenceCommand of type QString ***

    static const QString defaultCopyReferenceCommand;
    static const QStringList availableCopyReferenceCommands;
    const QString &copyReferenceCommand();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setCopyReferenceCommand(const QString &copyReferenceCommand);
#endif // HAVE_KF5


    /// *** PageSize of type QPageSize::PageSizeId ***

    static const QPageSize::PageSizeId defaultPageSize;
    static const QVector<QPair<QPageSize::PageSizeId, QString>> availablePageSizes;
    QPageSize::PageSizeId pageSize();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setPageSize(const QPageSize::PageSizeId pageSize);
#endif // HAVE_KF5


    /// *** BackupScope of type BackupScope ***

    static const BackupScope defaultBackupScope;
    static const QVector<QPair<Preferences::BackupScope, QString>> availableBackupScopes;
    BackupScope backupScope();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBackupScope(const BackupScope backupScope);
#endif // HAVE_KF5


    /// *** NumberOfBackups of type int ***

    static const int defaultNumberOfBackups;
    int numberOfBackups();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setNumberOfBackups(const int numberOfBackups);
#endif // HAVE_KF5


    /// *** IdSuggestionFormatStrings of type QStringList ***

    static const QStringList defaultIdSuggestionFormatStrings;
    const QStringList &idSuggestionFormatStrings();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setIdSuggestionFormatStrings(const QStringList &idSuggestionFormatStrings);
#endif // HAVE_KF5


    /// *** ActiveIdSuggestionFormatString of type QString ***

    static const QString defaultActiveIdSuggestionFormatString;
    const QString &activeIdSuggestionFormatString();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setActiveIdSuggestionFormatString(const QString &activeIdSuggestionFormatString);
#endif // HAVE_KF5


    /// *** LyXUseAutomaticPipeDetection of type bool ***

    static const bool defaultLyXUseAutomaticPipeDetection;
    bool lyXUseAutomaticPipeDetection();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setLyXUseAutomaticPipeDetection(const bool lyXUseAutomaticPipeDetection);
#endif // HAVE_KF5


    /// *** LyXPipePath of type QString ***

    static const QString defaultLyXPipePath;
    const QString &lyXPipePath();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setLyXPipePath(const QString &lyXPipePath);
#endif // HAVE_KF5


    /// *** BibTeXEncoding of type QString ***

    static const QString defaultBibTeXEncoding;
    static const QStringList availableBibTeXEncodings;
    const QString &bibTeXEncoding();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibTeXEncoding(const QString &bibTeXEncoding);
#endif // HAVE_KF5


    /// *** BibTeXStringDelimiter of type QString ***

    static const QString defaultBibTeXStringDelimiter;
    static const QStringList availableBibTeXStringDelimiters;
    const QString &bibTeXStringDelimiter();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibTeXStringDelimiter(const QString &bibTeXStringDelimiter);
#endif // HAVE_KF5


    /// *** BibTeXQuoteComment of type QuoteComment ***

    static const QuoteComment defaultBibTeXQuoteComment;
    static const QVector<QPair<Preferences::QuoteComment, QString>> availableBibTeXQuoteComments;
    QuoteComment bibTeXQuoteComment();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibTeXQuoteComment(const QuoteComment bibTeXQuoteComment);
#endif // HAVE_KF5


    /// *** BibTeXKeywordCasing of type KBibTeX::Casing ***

    static const KBibTeX::Casing defaultBibTeXKeywordCasing;
    static const QVector<QPair<KBibTeX::Casing, QString>> availableBibTeXKeywordCasings;
    KBibTeX::Casing bibTeXKeywordCasing();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibTeXKeywordCasing(const KBibTeX::Casing bibTeXKeywordCasing);
#endif // HAVE_KF5


    /// *** BibTeXProtectCasing of type bool ***

    static const bool defaultBibTeXProtectCasing;
    bool bibTeXProtectCasing();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibTeXProtectCasing(const bool bibTeXProtectCasing);
#endif // HAVE_KF5


    /// *** BibTeXListSeparator of type QString ***

    static const QString defaultBibTeXListSeparator;
    static const QStringList availableBibTeXListSeparators;
    const QString &bibTeXListSeparator();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibTeXListSeparator(const QString &bibTeXListSeparator);
#endif // HAVE_KF5


    /// *** LaTeXBabelLanguage of type QString ***

    static const QString defaultLaTeXBabelLanguage;
    const QString &laTeXBabelLanguage();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setLaTeXBabelLanguage(const QString &laTeXBabelLanguage);
#endif // HAVE_KF5


    /// *** BibTeXBibliographyStyle of type QString ***

    static const QString defaultBibTeXBibliographyStyle;
    const QString &bibTeXBibliographyStyle();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setBibTeXBibliographyStyle(const QString &bibTeXBibliographyStyle);
#endif // HAVE_KF5


    /// *** FileViewDoubleClickAction of type FileViewDoubleClickAction ***

    static const FileViewDoubleClickAction defaultFileViewDoubleClickAction;
    static const QVector<QPair<Preferences::FileViewDoubleClickAction, QString>> availableFileViewDoubleClickActions;
    FileViewDoubleClickAction fileViewDoubleClickAction();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setFileViewDoubleClickAction(const FileViewDoubleClickAction fileViewDoubleClickAction);
#endif // HAVE_KF5


    /// *** ColorCodes of type QVector<QPair<QColor, QString>> ***

    static const QVector<QPair<QColor, QString>> defaultColorCodes;
    const QVector<QPair<QColor, QString>> &colorCodes();
#ifdef HAVE_KF5
    /*!
     * @return true if this setting has been changed, i.e. the new value was different from the old value; false otherwise or under error conditions
     */
    bool setColorCodes(const QVector<QPair<QColor, QString>> &colorCodes);
#endif // HAVE_KF5

private:
    Q_DISABLE_COPY(Preferences)

    explicit Preferences();

    class Private;
    Private *const d;
};

#endif // KBIBTEX_CONFIG_PREFERENCES_H
