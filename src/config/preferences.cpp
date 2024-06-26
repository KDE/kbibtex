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

/// This file has been automatically generated using the script 'preferences-generator.py'
/// based on configuration data from file 'preferences.json'. If there are any problems or
/// bugs, you need to fix those two files and re-generated both 'preferences.h' and
/// 'preferences.cpp'. Manual changes in this file will be overwritten the next time the
/// script will be run. You have been warned.

#include <Preferences>

#include <set>

#include <QCoreApplication>
#ifdef HAVE_KFI18N
#include <KLocalizedString>
#else // HAVE_KFI18N
#define i18n(text) QStringLiteral(text)
#define i18nc(comment,text) QStringLiteral(text)
#endif // HAVE_KFI18N
#ifdef HAVE_KF
#include <KSharedConfig>
#include <KConfigWatcher>
#include <KConfigGroup>
#endif // HAVE_KF

#ifdef HAVE_KF
#include <NotificationHub>
#endif // HAVE_KF

#include <QVector>
#include <QDir>

class Preferences::Private
{
public:
#ifdef HAVE_KF
    KSharedConfigPtr config;
    KConfigWatcher::Ptr watcher;

    bool dirtyFlagBibliographySystem;
    Preferences::BibliographySystem cachedBibliographySystem;
    bool dirtyFlagPersonNameFormat;
    QString cachedPersonNameFormat;
    bool dirtyFlagCopyReferenceCommand;
    QString cachedCopyReferenceCommand;
    bool dirtyFlagPageSize;
    Preferences::PageSize cachedPageSize;
    bool dirtyFlagBackupScope;
    Preferences::BackupScope cachedBackupScope;
    bool dirtyFlagNumberOfBackups;
    int cachedNumberOfBackups;
    bool dirtyFlagIdSuggestionFormatStrings;
    QStringList cachedIdSuggestionFormatStrings;
    bool dirtyFlagActiveIdSuggestionFormatString;
    QString cachedActiveIdSuggestionFormatString;
    bool dirtyFlagLyXUseAutomaticPipeDetection;
    bool cachedLyXUseAutomaticPipeDetection;
    bool dirtyFlagLyXPipePath;
    QString cachedLyXPipePath;
    bool dirtyFlagBibTeXEncoding;
    QString cachedBibTeXEncoding;
    bool dirtyFlagBibTeXStringDelimiter;
    QString cachedBibTeXStringDelimiter;
    bool dirtyFlagBibTeXCommentContext;
    Preferences::CommentContext cachedBibTeXCommentContext;
    bool dirtyFlagBibTeXCommentPrefix;
    QString cachedBibTeXCommentPrefix;
    bool dirtyFlagBibTeXKeywordCasing;
    KBibTeX::Casing cachedBibTeXKeywordCasing;
    bool dirtyFlagBibTeXProtectCasing;
    bool cachedBibTeXProtectCasing;
    bool dirtyFlagBibTeXListSeparator;
    QString cachedBibTeXListSeparator;
    bool dirtyFlagbibTeXEntriesSortedByIdentifier;
    bool cachedbibTeXEntriesSortedByIdentifier;
    bool dirtyFlagLaTeXBabelLanguage;
    QString cachedLaTeXBabelLanguage;
    bool dirtyFlagBibTeXBibliographyStyle;
    QString cachedBibTeXBibliographyStyle;
    bool dirtyFlagFileViewDoubleClickAction;
    Preferences::FileViewDoubleClickAction cachedFileViewDoubleClickAction;
    bool dirtyFlagColorCodes;
    QVector<QPair<QString, QString>> cachedColorCodes;
#endif // HAVE_KF

    Private(Preferences *)
    {
#ifdef HAVE_KF
        config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));
        watcher = KConfigWatcher::create(config);
        dirtyFlagBibliographySystem = true;
        cachedBibliographySystem = Preferences::defaultBibliographySystem;
        dirtyFlagPersonNameFormat = true;
        cachedPersonNameFormat = Preferences::defaultPersonNameFormat;
        dirtyFlagCopyReferenceCommand = true;
        cachedCopyReferenceCommand = Preferences::defaultCopyReferenceCommand;
        dirtyFlagPageSize = true;
        cachedPageSize = Preferences::defaultPageSize;
        dirtyFlagBackupScope = true;
        cachedBackupScope = Preferences::defaultBackupScope;
        dirtyFlagNumberOfBackups = true;
        cachedNumberOfBackups = Preferences::defaultNumberOfBackups;
        dirtyFlagIdSuggestionFormatStrings = true;
        cachedIdSuggestionFormatStrings = Preferences::defaultIdSuggestionFormatStrings;
        dirtyFlagActiveIdSuggestionFormatString = true;
        cachedActiveIdSuggestionFormatString = Preferences::defaultActiveIdSuggestionFormatString;
        dirtyFlagLyXUseAutomaticPipeDetection = true;
        cachedLyXUseAutomaticPipeDetection = Preferences::defaultLyXUseAutomaticPipeDetection;
        dirtyFlagLyXPipePath = true;
        cachedLyXPipePath = Preferences::defaultLyXPipePath;
        dirtyFlagBibTeXEncoding = true;
        cachedBibTeXEncoding = Preferences::defaultBibTeXEncoding;
        dirtyFlagBibTeXStringDelimiter = true;
        cachedBibTeXStringDelimiter = Preferences::defaultBibTeXStringDelimiter;
        dirtyFlagBibTeXCommentContext = true;
        cachedBibTeXCommentContext = Preferences::defaultBibTeXCommentContext;
        dirtyFlagBibTeXCommentPrefix = true;
        cachedBibTeXCommentPrefix = Preferences::defaultBibTeXCommentPrefix;
        dirtyFlagBibTeXKeywordCasing = true;
        cachedBibTeXKeywordCasing = Preferences::defaultBibTeXKeywordCasing;
        dirtyFlagBibTeXProtectCasing = true;
        cachedBibTeXProtectCasing = Preferences::defaultBibTeXProtectCasing;
        dirtyFlagBibTeXListSeparator = true;
        cachedBibTeXListSeparator = Preferences::defaultBibTeXListSeparator;
        dirtyFlagbibTeXEntriesSortedByIdentifier = true;
        cachedbibTeXEntriesSortedByIdentifier = Preferences::defaultbibTeXEntriesSortedByIdentifier;
        dirtyFlagLaTeXBabelLanguage = true;
        cachedLaTeXBabelLanguage = Preferences::defaultLaTeXBabelLanguage;
        dirtyFlagBibTeXBibliographyStyle = true;
        cachedBibTeXBibliographyStyle = Preferences::defaultBibTeXBibliographyStyle;
        dirtyFlagFileViewDoubleClickAction = true;
        cachedFileViewDoubleClickAction = Preferences::defaultFileViewDoubleClickAction;
        dirtyFlagColorCodes = true;
        cachedColorCodes = Preferences::defaultColorCodes;
#endif // HAVE_KF
    }

#ifdef HAVE_KF
    inline bool validateValueForBibliographySystem(const Preferences::BibliographySystem valueToBeChecked) {
        for (QVector<QPair<Preferences::BibliographySystem, QString>>::ConstIterator it = Preferences::availableBibliographySystems.constBegin(); it != Preferences::availableBibliographySystems.constEnd(); ++it)
            if (it->first == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForPersonNameFormat(const QString &valueToBeChecked) {
        return valueToBeChecked.contains(QStringLiteral("%f")) && valueToBeChecked.contains(QStringLiteral("%l")) && valueToBeChecked.contains(QStringLiteral("%s"));
    }

    inline bool validateValueForCopyReferenceCommand(const QString &valueToBeChecked) {
        return valueToBeChecked.isEmpty() || Preferences::availableCopyReferenceCommands.contains(valueToBeChecked);
    }

    inline bool validateValueForPageSize(const Preferences::PageSize valueToBeChecked) {
        for (QVector<QPair<Preferences::PageSize, QString>>::ConstIterator it = Preferences::availablePageSizes.constBegin(); it != Preferences::availablePageSizes.constEnd(); ++it)
            if (it->first == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForBackupScope(const Preferences::BackupScope valueToBeChecked) {
        for (QVector<QPair<Preferences::BackupScope, QString>>::ConstIterator it = Preferences::availableBackupScopes.constBegin(); it != Preferences::availableBackupScopes.constEnd(); ++it)
            if (it->first == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForNumberOfBackups(const int valueToBeChecked) {
        return valueToBeChecked >= 0;
    }

    inline bool validateValueForIdSuggestionFormatStrings(const QStringList &valueToBeChecked) {
        return !valueToBeChecked.isEmpty() && (valueToBeChecked.front().contains(QLatin1Char('A')) || valueToBeChecked.front().contains(QLatin1Char('a')) || valueToBeChecked.front().contains(QLatin1Char('y')) || valueToBeChecked.front().contains(QLatin1Char('Y')) || valueToBeChecked.front().contains(QLatin1Char('T')));
    }

    inline bool validateValueForActiveIdSuggestionFormatString(const QString &valueToBeChecked) {
        Q_UNUSED(valueToBeChecked)
        return true;
    }

    inline bool validateValueForLyXUseAutomaticPipeDetection(const bool valueToBeChecked) {
        Q_UNUSED(valueToBeChecked)
        return true;
    }

    inline bool validateValueForLyXPipePath(const QString &valueToBeChecked) {
        return valueToBeChecked.startsWith(QLatin1Char('/'));
    }

    inline bool validateValueForBibTeXEncoding(const QString &valueToBeChecked) {
        return Preferences::availableBibTeXEncodings.contains(valueToBeChecked);
    }

    inline bool validateValueForBibTeXStringDelimiter(const QString &valueToBeChecked) {
        return Preferences::availableBibTeXStringDelimiters.contains(valueToBeChecked);
    }

    inline bool validateValueForBibTeXCommentContext(const Preferences::CommentContext valueToBeChecked) {
        for (QVector<QPair<Preferences::CommentContext, QString>>::ConstIterator it = Preferences::availableBibTeXCommentContexts.constBegin(); it != Preferences::availableBibTeXCommentContexts.constEnd(); ++it)
            if (it->first == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForBibTeXCommentPrefix(const QString &valueToBeChecked) {
        return valueToBeChecked.isEmpty() || valueToBeChecked.startsWith(QStringLiteral("%"));
    }

    inline bool validateValueForBibTeXKeywordCasing(const KBibTeX::Casing valueToBeChecked) {
        for (QVector<QPair<KBibTeX::Casing, QString>>::ConstIterator it = Preferences::availableBibTeXKeywordCasings.constBegin(); it != Preferences::availableBibTeXKeywordCasings.constEnd(); ++it)
            if (it->first == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForBibTeXProtectCasing(const bool valueToBeChecked) {
        Q_UNUSED(valueToBeChecked)
        return true;
    }

    inline bool validateValueForBibTeXListSeparator(const QString &valueToBeChecked) {
        return Preferences::availableBibTeXListSeparators.contains(valueToBeChecked);
    }

    inline bool validateValueForbibTeXEntriesSortedByIdentifier(const bool valueToBeChecked) {
        Q_UNUSED(valueToBeChecked)
        return true;
    }

    inline bool validateValueForLaTeXBabelLanguage(const QString &valueToBeChecked) {
        return !valueToBeChecked.isEmpty();
    }

    inline bool validateValueForBibTeXBibliographyStyle(const QString &valueToBeChecked) {
        return !valueToBeChecked.isEmpty();
    }

    inline bool validateValueForFileViewDoubleClickAction(const Preferences::FileViewDoubleClickAction valueToBeChecked) {
        for (QVector<QPair<Preferences::FileViewDoubleClickAction, QString>>::ConstIterator it = Preferences::availableFileViewDoubleClickActions.constBegin(); it != Preferences::availableFileViewDoubleClickActions.constEnd(); ++it)
            if (it->first == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForColorCodes(const QVector<QPair<QString, QString>> &valueToBeChecked) {
        static const QString white{QStringLiteral("#ffffff")}, black{QStringLiteral("#000000")};
        for (QVector<QPair<QString, QString>>::ConstIterator it = valueToBeChecked.constBegin(); it != valueToBeChecked.constEnd(); ++it)
            if (it->first.isEmpty() || it->second.isEmpty() || it->first == white || it->first == black)
                return false;
        return true;
    }

    QVector<QPair<QString, QString>> readEntryColorCodes(const KConfigGroup &configGroup, const QString &key) const
    {
        const QString rawEntry = configGroup.readEntry(key, QString());
        if (rawEntry.isEmpty()) return Preferences::defaultColorCodes;
#if QT_VERSION >= 0x050e00
        const QStringList pairs = rawEntry.split(QStringLiteral("\0\0"), Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
        const QStringList pairs = rawEntry.split(QStringLiteral("\0\0"), QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
        if (pairs.isEmpty()) return Preferences::defaultColorCodes;
        QVector<QPair<QString, QString>> result;
        for (const QString &pair : pairs) {
#if QT_VERSION >= 0x050e00
            const QStringList colorLabelPair = pair.split(QStringLiteral("\0"), Qt::SkipEmptyParts);
#else // QT_VERSION < 0x050e00
            const QStringList colorLabelPair = pair.split(QStringLiteral("\0"), QString::SkipEmptyParts);
#endif // QT_VERSION >= 0x050e00
            if (colorLabelPair.length() != 2) return Preferences::defaultColorCodes;
            result.append(qMakePair(colorLabelPair[0], colorLabelPair[1]));
        }
        return result;
    }

    void writeEntryColorCodes(KConfigGroup &configGroup, const QString &key, const QVector<QPair<QString, QString>> &valueToBeWritten)
    {
        QString rawEntry;
        for (QVector<QPair<QString, QString>>::ConstIterator it = valueToBeWritten.constBegin(); it != valueToBeWritten.constEnd(); ++it) {
            if (!rawEntry.isEmpty()) rawEntry.append(QStringLiteral("\0\0"));
            rawEntry = rawEntry.append(it->first).append(QStringLiteral("\0")).append(it->second);
        }
        configGroup.writeEntry(key, rawEntry, KConfig::Notify);
    }
#endif // HAVE_KF
};

Preferences &Preferences::instance()
{
    static Preferences singleton;
    return singleton;
}

Preferences::Preferences()
        : d(new Preferences::Private(this))
{
#ifdef HAVE_KF
    QObject::connect(d->watcher.data(), &KConfigWatcher::configChanged, QCoreApplication::instance(), [this](const KConfigGroup &group, const QByteArrayList &names) {
        std::set<int> eventsToPublish;
        if (group.name() == QStringLiteral("General") && names.contains("BibliographySystem")) {
            /// Configuration setting BibliographySystem got changed by another Preferences instance";
            d->dirtyFlagBibliographySystem = true;
            eventsToPublish.insert(NotificationHub::EventBibliographySystemChanged);
        }
        if (group.name() == QStringLiteral("General") && names.contains("PersonNameFormat")) {
            /// Configuration setting PersonNameFormat got changed by another Preferences instance";
            d->dirtyFlagPersonNameFormat = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("LaTeX") && names.contains("CopyReferenceCommand")) {
            /// Configuration setting CopyReferenceCommand got changed by another Preferences instance";
            d->dirtyFlagCopyReferenceCommand = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("General") && names.contains("PageSize")) {
            /// Configuration setting PageSize got changed by another Preferences instance";
            d->dirtyFlagPageSize = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("InputOutput") && names.contains("BackupScope")) {
            /// Configuration setting BackupScope got changed by another Preferences instance";
            d->dirtyFlagBackupScope = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("InputOutput") && names.contains("NumberOfBackups")) {
            /// Configuration setting NumberOfBackups got changed by another Preferences instance";
            d->dirtyFlagNumberOfBackups = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("IdSuggestions") && names.contains("IdSuggestionFormatStrings")) {
            /// Configuration setting IdSuggestionFormatStrings got changed by another Preferences instance";
            d->dirtyFlagIdSuggestionFormatStrings = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("IdSuggestions") && names.contains("ActiveIdSuggestionFormatString")) {
            /// Configuration setting ActiveIdSuggestionFormatString got changed by another Preferences instance";
            d->dirtyFlagActiveIdSuggestionFormatString = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("LyX") && names.contains("LyXUseAutomaticPipeDetection")) {
            /// Configuration setting LyXUseAutomaticPipeDetection got changed by another Preferences instance";
            d->dirtyFlagLyXUseAutomaticPipeDetection = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("LyX") && names.contains("LyXPipePath")) {
            /// Configuration setting LyXPipePath got changed by another Preferences instance";
            d->dirtyFlagLyXPipePath = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("BibTeXEncoding")) {
            /// Configuration setting BibTeXEncoding got changed by another Preferences instance";
            d->dirtyFlagBibTeXEncoding = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("BibTeXStringDelimiter")) {
            /// Configuration setting BibTeXStringDelimiter got changed by another Preferences instance";
            d->dirtyFlagBibTeXStringDelimiter = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("BibTeXCommentContext")) {
            /// Configuration setting BibTeXCommentContext got changed by another Preferences instance";
            d->dirtyFlagBibTeXCommentContext = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("BibTeXCommentPrefix")) {
            /// Configuration setting BibTeXCommentPrefix got changed by another Preferences instance";
            d->dirtyFlagBibTeXCommentPrefix = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("BibTeXKeywordCasing")) {
            /// Configuration setting BibTeXKeywordCasing got changed by another Preferences instance";
            d->dirtyFlagBibTeXKeywordCasing = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("BibTeXProtectCasing")) {
            /// Configuration setting BibTeXProtectCasing got changed by another Preferences instance";
            d->dirtyFlagBibTeXProtectCasing = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("BibTeXListSeparator")) {
            /// Configuration setting BibTeXListSeparator got changed by another Preferences instance";
            d->dirtyFlagBibTeXListSeparator = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterBibTeX") && names.contains("bibTeXEntriesSortedByIdentifier")) {
            /// Configuration setting bibTeXEntriesSortedByIdentifier got changed by another Preferences instance";
            d->dirtyFlagbibTeXEntriesSortedByIdentifier = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterLaTeXbased") && names.contains("LaTeXBabelLanguage")) {
            /// Configuration setting LaTeXBabelLanguage got changed by another Preferences instance";
            d->dirtyFlagLaTeXBabelLanguage = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("FileExporterLaTeXbased") && names.contains("BibTeXBibliographyStyle")) {
            /// Configuration setting BibTeXBibliographyStyle got changed by another Preferences instance";
            d->dirtyFlagBibTeXBibliographyStyle = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("User Interface") && names.contains("FileViewDoubleClickAction")) {
            /// Configuration setting FileViewDoubleClickAction got changed by another Preferences instance";
            d->dirtyFlagFileViewDoubleClickAction = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }
        if (group.name() == QStringLiteral("Color Labels") && names.contains("ColorCodes")) {
            /// Configuration setting ColorCodes got changed by another Preferences instance";
            d->dirtyFlagColorCodes = true;
            eventsToPublish.insert(NotificationHub::EventConfigurationChanged);
        }

        for (const int eventId : eventsToPublish)
            NotificationHub::publishEvent(eventId);
    });
#endif // HAVE_KF
}

Preferences::~Preferences()
{
    delete d;
}

const QVector<QPair<Preferences::BibliographySystem, QString>> Preferences::availableBibliographySystems {{Preferences::BibliographySystem::BibTeX, i18n("BibTeX")}, {Preferences::BibliographySystem::BibLaTeX, i18n("BibLaTeX")}};
const Preferences::BibliographySystem Preferences::defaultBibliographySystem = Preferences::availableBibliographySystems.front().first;

Preferences::BibliographySystem Preferences::bibliographySystem()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibliographySystem) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("General"));
        const Preferences::BibliographySystem valueFromConfig = static_cast<Preferences::BibliographySystem>(configGroup.readEntry(QStringLiteral("BibliographySystem"), static_cast<int>(Preferences::defaultBibliographySystem)));
        if (d->validateValueForBibliographySystem(valueFromConfig)) {
            d->cachedBibliographySystem = valueFromConfig;
            d->dirtyFlagBibliographySystem = false;
        } else {
            /// Configuration file setting for BibliographySystem has an invalid value, using default as fallback
            setBibliographySystem(Preferences::defaultBibliographySystem);
        }
    }
    return d->cachedBibliographySystem;
#else // HAVE_KF
    return defaultBibliographySystem;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibliographySystem(const Preferences::BibliographySystem newValue)
{
    if (!d->validateValueForBibliographySystem(newValue)) return false;
    d->dirtyFlagBibliographySystem = false;
    d->cachedBibliographySystem = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("General"));
    const Preferences::BibliographySystem valueFromConfig = static_cast<Preferences::BibliographySystem>(configGroup.readEntry(QStringLiteral("BibliographySystem"), static_cast<int>(Preferences::defaultBibliographySystem)));
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("BibliographySystem"), static_cast<int>(newValue), KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QString Preferences::personNameFormatLastFirst = QStringLiteral("<%l><, %s><, %f>");
const QString Preferences::personNameFormatFirstLast = QStringLiteral("<%f ><%l>< %s>");
const QString Preferences::defaultPersonNameFormat = Preferences::personNameFormatLastFirst;

const QString &Preferences::personNameFormat()
{
#ifdef HAVE_KF
    if (d->dirtyFlagPersonNameFormat) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("General"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("PersonNameFormat"), Preferences::defaultPersonNameFormat);
        if (d->validateValueForPersonNameFormat(valueFromConfig)) {
            d->cachedPersonNameFormat = valueFromConfig;
            d->dirtyFlagPersonNameFormat = false;
        } else {
            /// Configuration file setting for PersonNameFormat has an invalid value, using default as fallback
            setPersonNameFormat(Preferences::defaultPersonNameFormat);
        }
    }
    return d->cachedPersonNameFormat;
#else // HAVE_KF
    return defaultPersonNameFormat;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setPersonNameFormat(const QString &newValue)
{
    if (!d->validateValueForPersonNameFormat(newValue)) return false;
    d->dirtyFlagPersonNameFormat = false;
    d->cachedPersonNameFormat = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("General"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("PersonNameFormat"), Preferences::defaultPersonNameFormat);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("PersonNameFormat"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QStringList Preferences::availableCopyReferenceCommands {QStringLiteral("cite"), QStringLiteral("citealt"), QStringLiteral("citeauthor"), QStringLiteral("citeauthor*"), QStringLiteral("citeyear"), QStringLiteral("citeyearpar"), QStringLiteral("shortcite"), QStringLiteral("citet"), QStringLiteral("citet*"), QStringLiteral("citep"), QStringLiteral("citep*")};
const QString Preferences::defaultCopyReferenceCommand = Preferences::availableCopyReferenceCommands.front();

const QString &Preferences::copyReferenceCommand()
{
#ifdef HAVE_KF
    if (d->dirtyFlagCopyReferenceCommand) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("LaTeX"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("CopyReferenceCommand"), Preferences::defaultCopyReferenceCommand);
        if (d->validateValueForCopyReferenceCommand(valueFromConfig)) {
            d->cachedCopyReferenceCommand = valueFromConfig;
            d->dirtyFlagCopyReferenceCommand = false;
        } else {
            /// Configuration file setting for CopyReferenceCommand has an invalid value, using default as fallback
            setCopyReferenceCommand(Preferences::defaultCopyReferenceCommand);
        }
    }
    return d->cachedCopyReferenceCommand;
#else // HAVE_KF
    return defaultCopyReferenceCommand;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setCopyReferenceCommand(const QString &newValue)
{
    QString sanitizedNewValue = newValue;
    const QString lowerSanitizedNewValue = sanitizedNewValue.toLower();
    for (const QString &knownCopyReferenceCommand : availableCopyReferenceCommands)
        if (knownCopyReferenceCommand.toLower() == lowerSanitizedNewValue) {
            sanitizedNewValue = knownCopyReferenceCommand;
            break;
        }
    if (!d->validateValueForCopyReferenceCommand(sanitizedNewValue)) return false;
    d->dirtyFlagCopyReferenceCommand = false;
    d->cachedCopyReferenceCommand = sanitizedNewValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("LaTeX"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("CopyReferenceCommand"), Preferences::defaultCopyReferenceCommand);
    if (valueFromConfig == sanitizedNewValue) return false;
    configGroup.writeEntry(QStringLiteral("CopyReferenceCommand"), sanitizedNewValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QVector<QPair<Preferences::PageSize, QString>> Preferences::availablePageSizes {{Preferences::PageSize::A4, QStringLiteral("a4paper")}, {Preferences::PageSize::Letter, QStringLiteral("letter")}, {Preferences::PageSize::Legal, QStringLiteral("legal")}};
const Preferences::PageSize Preferences::defaultPageSize = Preferences::availablePageSizes.front().first;

Preferences::PageSize Preferences::pageSize()
{
#ifdef HAVE_KF
    if (d->dirtyFlagPageSize) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("General"));
        const Preferences::PageSize valueFromConfig = static_cast<Preferences::PageSize>(configGroup.readEntry(QStringLiteral("PageSize"), static_cast<int>(Preferences::defaultPageSize)));
        if (d->validateValueForPageSize(valueFromConfig)) {
            d->cachedPageSize = valueFromConfig;
            d->dirtyFlagPageSize = false;
        } else {
            /// Configuration file setting for PageSize has an invalid value, using default as fallback
            setPageSize(Preferences::defaultPageSize);
        }
    }
    return d->cachedPageSize;
#else // HAVE_KF
    return defaultPageSize;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setPageSize(const Preferences::PageSize newValue)
{
    if (!d->validateValueForPageSize(newValue)) return false;
    d->dirtyFlagPageSize = false;
    d->cachedPageSize = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("General"));
    const Preferences::PageSize valueFromConfig = static_cast<Preferences::PageSize>(configGroup.readEntry(QStringLiteral("PageSize"), static_cast<int>(Preferences::defaultPageSize)));
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("PageSize"), static_cast<int>(newValue), KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QVector<QPair<Preferences::BackupScope, QString>> Preferences::availableBackupScopes {{Preferences::BackupScope::None, i18n("No backups")}, {Preferences::BackupScope::LocalOnly, i18n("Local files only")}, {Preferences::BackupScope::BothLocalAndRemote, i18n("Both local and remote files")}};
const Preferences::BackupScope Preferences::defaultBackupScope = Preferences::availableBackupScopes.front().first;

Preferences::BackupScope Preferences::backupScope()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBackupScope) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("InputOutput"));
        const Preferences::BackupScope valueFromConfig = static_cast<Preferences::BackupScope>(configGroup.readEntry(QStringLiteral("BackupScope"), static_cast<int>(Preferences::defaultBackupScope)));
        if (d->validateValueForBackupScope(valueFromConfig)) {
            d->cachedBackupScope = valueFromConfig;
            d->dirtyFlagBackupScope = false;
        } else {
            /// Configuration file setting for BackupScope has an invalid value, using default as fallback
            setBackupScope(Preferences::defaultBackupScope);
        }
    }
    return d->cachedBackupScope;
#else // HAVE_KF
    return defaultBackupScope;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBackupScope(const Preferences::BackupScope newValue)
{
    if (!d->validateValueForBackupScope(newValue)) return false;
    d->dirtyFlagBackupScope = false;
    d->cachedBackupScope = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("InputOutput"));
    const Preferences::BackupScope valueFromConfig = static_cast<Preferences::BackupScope>(configGroup.readEntry(QStringLiteral("BackupScope"), static_cast<int>(Preferences::defaultBackupScope)));
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("BackupScope"), static_cast<int>(newValue), KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const int Preferences::defaultNumberOfBackups = 5;

int Preferences::numberOfBackups()
{
#ifdef HAVE_KF
    if (d->dirtyFlagNumberOfBackups) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("InputOutput"));
        const int valueFromConfig = configGroup.readEntry(QStringLiteral("NumberOfBackups"), Preferences::defaultNumberOfBackups);
        if (d->validateValueForNumberOfBackups(valueFromConfig)) {
            d->cachedNumberOfBackups = valueFromConfig;
            d->dirtyFlagNumberOfBackups = false;
        } else {
            /// Configuration file setting for NumberOfBackups has an invalid value, using default as fallback
            setNumberOfBackups(Preferences::defaultNumberOfBackups);
        }
    }
    return d->cachedNumberOfBackups;
#else // HAVE_KF
    return defaultNumberOfBackups;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setNumberOfBackups(const int newValue)
{
    if (!d->validateValueForNumberOfBackups(newValue)) return false;
    d->dirtyFlagNumberOfBackups = false;
    d->cachedNumberOfBackups = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("InputOutput"));
    const int valueFromConfig = configGroup.readEntry(QStringLiteral("NumberOfBackups"), Preferences::defaultNumberOfBackups);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("NumberOfBackups"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QStringList Preferences::defaultIdSuggestionFormatStrings {QStringLiteral("A"), QStringLiteral("A2|y"), QStringLiteral("A3|y"), QStringLiteral("A4|y|\":|T5"), QStringLiteral("al|\":|T"), QStringLiteral("al|y"), QStringLiteral("al|Y"), QStringLiteral("Al\"-|\"-|y"), QStringLiteral("Al\"+|Y"), QStringLiteral("al|y|T"), QStringLiteral("al|Y|T3"), QStringLiteral("al|Y|T3l"), QStringLiteral("a|\":|Y|\":|T1"), QStringLiteral("a|y"), QStringLiteral("A|\":|Y")};

const QStringList &Preferences::idSuggestionFormatStrings()
{
#ifdef HAVE_KF
    if (d->dirtyFlagIdSuggestionFormatStrings) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("IdSuggestions"));
        const QStringList valueFromConfig = configGroup.readEntry(QStringLiteral("IdSuggestionFormatStrings"), Preferences::defaultIdSuggestionFormatStrings);
        if (d->validateValueForIdSuggestionFormatStrings(valueFromConfig)) {
            d->cachedIdSuggestionFormatStrings = valueFromConfig;
            d->dirtyFlagIdSuggestionFormatStrings = false;
        } else {
            /// Configuration file setting for IdSuggestionFormatStrings has an invalid value, using default as fallback
            setIdSuggestionFormatStrings(Preferences::defaultIdSuggestionFormatStrings);
        }
    }
    return d->cachedIdSuggestionFormatStrings;
#else // HAVE_KF
    return defaultIdSuggestionFormatStrings;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setIdSuggestionFormatStrings(const QStringList &newValue)
{
    if (!d->validateValueForIdSuggestionFormatStrings(newValue)) return false;
    d->dirtyFlagIdSuggestionFormatStrings = false;
    d->cachedIdSuggestionFormatStrings = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("IdSuggestions"));
    const QStringList valueFromConfig = configGroup.readEntry(QStringLiteral("IdSuggestionFormatStrings"), Preferences::defaultIdSuggestionFormatStrings);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("IdSuggestionFormatStrings"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QString Preferences::defaultActiveIdSuggestionFormatString {};

const QString &Preferences::activeIdSuggestionFormatString()
{
#ifdef HAVE_KF
    if (d->dirtyFlagActiveIdSuggestionFormatString) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("IdSuggestions"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("ActiveIdSuggestionFormatString"), Preferences::defaultActiveIdSuggestionFormatString);
        if (d->validateValueForActiveIdSuggestionFormatString(valueFromConfig)) {
            d->cachedActiveIdSuggestionFormatString = valueFromConfig;
            d->dirtyFlagActiveIdSuggestionFormatString = false;
        } else {
            /// Configuration file setting for ActiveIdSuggestionFormatString has an invalid value, using default as fallback
            setActiveIdSuggestionFormatString(Preferences::defaultActiveIdSuggestionFormatString);
        }
    }
    return d->cachedActiveIdSuggestionFormatString;
#else // HAVE_KF
    return defaultActiveIdSuggestionFormatString;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setActiveIdSuggestionFormatString(const QString &newValue)
{
    if (!d->validateValueForActiveIdSuggestionFormatString(newValue)) return false;
    d->dirtyFlagActiveIdSuggestionFormatString = false;
    d->cachedActiveIdSuggestionFormatString = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("IdSuggestions"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("ActiveIdSuggestionFormatString"), Preferences::defaultActiveIdSuggestionFormatString);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("ActiveIdSuggestionFormatString"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const bool Preferences::defaultLyXUseAutomaticPipeDetection = true;

bool Preferences::lyXUseAutomaticPipeDetection()
{
#ifdef HAVE_KF
    if (d->dirtyFlagLyXUseAutomaticPipeDetection) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("LyX"));
        const bool valueFromConfig = configGroup.readEntry(QStringLiteral("LyXUseAutomaticPipeDetection"), Preferences::defaultLyXUseAutomaticPipeDetection);
        if (d->validateValueForLyXUseAutomaticPipeDetection(valueFromConfig)) {
            d->cachedLyXUseAutomaticPipeDetection = valueFromConfig;
            d->dirtyFlagLyXUseAutomaticPipeDetection = false;
        } else {
            /// Configuration file setting for LyXUseAutomaticPipeDetection has an invalid value, using default as fallback
            setLyXUseAutomaticPipeDetection(Preferences::defaultLyXUseAutomaticPipeDetection);
        }
    }
    return d->cachedLyXUseAutomaticPipeDetection;
#else // HAVE_KF
    return defaultLyXUseAutomaticPipeDetection;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setLyXUseAutomaticPipeDetection(const bool newValue)
{
    if (!d->validateValueForLyXUseAutomaticPipeDetection(newValue)) return false;
    d->dirtyFlagLyXUseAutomaticPipeDetection = false;
    d->cachedLyXUseAutomaticPipeDetection = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("LyX"));
    const bool valueFromConfig = configGroup.readEntry(QStringLiteral("LyXUseAutomaticPipeDetection"), Preferences::defaultLyXUseAutomaticPipeDetection);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("LyXUseAutomaticPipeDetection"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QString Preferences::defaultLyXPipePath = QDir::homePath() + QStringLiteral("/.lyxpipe.in");

const QString &Preferences::lyXPipePath()
{
#ifdef HAVE_KF
    if (d->dirtyFlagLyXPipePath) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("LyX"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("LyXPipePath"), Preferences::defaultLyXPipePath);
        if (d->validateValueForLyXPipePath(valueFromConfig)) {
            d->cachedLyXPipePath = valueFromConfig;
            d->dirtyFlagLyXPipePath = false;
        } else {
            /// Configuration file setting for LyXPipePath has an invalid value, using default as fallback
            setLyXPipePath(Preferences::defaultLyXPipePath);
        }
    }
    return d->cachedLyXPipePath;
#else // HAVE_KF
    return defaultLyXPipePath;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setLyXPipePath(const QString &newValue)
{
    QString sanitizedNewValue = newValue;
    if (sanitizedNewValue.endsWith(QStringLiteral(".out")))
        sanitizedNewValue = sanitizedNewValue.left(sanitizedNewValue.length() - 4);
    if (!sanitizedNewValue.endsWith(QStringLiteral(".in")))
        sanitizedNewValue = sanitizedNewValue.append(QStringLiteral(".in"));
    if (!d->validateValueForLyXPipePath(sanitizedNewValue)) return false;
    d->dirtyFlagLyXPipePath = false;
    d->cachedLyXPipePath = sanitizedNewValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("LyX"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("LyXPipePath"), Preferences::defaultLyXPipePath);
    if (valueFromConfig == sanitizedNewValue) return false;
    configGroup.writeEntry(QStringLiteral("LyXPipePath"), sanitizedNewValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QStringList Preferences::availableBibTeXEncodings {QStringLiteral("LaTeX"), QStringLiteral("ISO-8859-1"), QStringLiteral("ISO-8859-2"), QStringLiteral("ISO-8859-3"), QStringLiteral("ISO-8859-4"), QStringLiteral("ISO-8859-5"), QStringLiteral("ISO-8859-6"), QStringLiteral("ISO-8859-7"), QStringLiteral("ISO-8859-8"), QStringLiteral("ISO-8859-9"), QStringLiteral("ISO-8859-10"), QStringLiteral("ISO-8859-13"), QStringLiteral("ISO-8859-14"), QStringLiteral("ISO-8859-15"), QStringLiteral("ISO-8859-16"), QStringLiteral("UTF-8"), QStringLiteral("UTF-16"), QStringLiteral("UTF-16BE"), QStringLiteral("UTF-16LE"), QStringLiteral("UTF-32"), QStringLiteral("UTF-32BE"), QStringLiteral("UTF-32LE"), QStringLiteral("KOI8-R"), QStringLiteral("KOI8-U"), QStringLiteral("Big5"), QStringLiteral("Big5-HKSCS"), QStringLiteral("GB18030"), QStringLiteral("EUC-JP"), QStringLiteral("EUC-KR"), QStringLiteral("ISO 2022-JP"), QStringLiteral("Shift-JIS"), QStringLiteral("Windows-949"), QStringLiteral("Windows-1250"), QStringLiteral("Windows-1251"), QStringLiteral("Windows-1252"), QStringLiteral("Windows-1253"), QStringLiteral("Windows-1254"), QStringLiteral("Windows-1255"), QStringLiteral("Windows-1256"), QStringLiteral("Windows-1257"), QStringLiteral("Windows-1258")};
const QString Preferences::defaultBibTeXEncoding = QStringLiteral("UTF-8");

const QString &Preferences::bibTeXEncoding()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXEncoding) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXEncoding"), Preferences::defaultBibTeXEncoding);
        if (d->validateValueForBibTeXEncoding(valueFromConfig)) {
            d->cachedBibTeXEncoding = valueFromConfig;
            d->dirtyFlagBibTeXEncoding = false;
        } else {
            /// Configuration file setting for BibTeXEncoding has an invalid value, using default as fallback
            setBibTeXEncoding(Preferences::defaultBibTeXEncoding);
        }
    }
    return d->cachedBibTeXEncoding;
#else // HAVE_KF
    return defaultBibTeXEncoding;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXEncoding(const QString &newValue)
{
    QString sanitizedNewValue = newValue;
    const QString lowerSanitizedNewValue = sanitizedNewValue.toLower();
    for (const QString &knownBibTeXEncoding : availableBibTeXEncodings)
        if (knownBibTeXEncoding.toLower() == lowerSanitizedNewValue) {
            sanitizedNewValue = knownBibTeXEncoding;
            break;
        }
    if (!d->validateValueForBibTeXEncoding(sanitizedNewValue)) return false;
    d->dirtyFlagBibTeXEncoding = false;
    d->cachedBibTeXEncoding = sanitizedNewValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXEncoding"), Preferences::defaultBibTeXEncoding);
    if (valueFromConfig == sanitizedNewValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXEncoding"), sanitizedNewValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QStringList Preferences::availableBibTeXStringDelimiters {QStringLiteral("{}"), QStringLiteral("\"\""), QStringLiteral("()")};
const QString Preferences::defaultBibTeXStringDelimiter = Preferences::availableBibTeXStringDelimiters.front();

const QString &Preferences::bibTeXStringDelimiter()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXStringDelimiter) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXStringDelimiter"), Preferences::defaultBibTeXStringDelimiter);
        if (d->validateValueForBibTeXStringDelimiter(valueFromConfig)) {
            d->cachedBibTeXStringDelimiter = valueFromConfig;
            d->dirtyFlagBibTeXStringDelimiter = false;
        } else {
            /// Configuration file setting for BibTeXStringDelimiter has an invalid value, using default as fallback
            setBibTeXStringDelimiter(Preferences::defaultBibTeXStringDelimiter);
        }
    }
    return d->cachedBibTeXStringDelimiter;
#else // HAVE_KF
    return defaultBibTeXStringDelimiter;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXStringDelimiter(const QString &newValue)
{
    QString sanitizedNewValue = newValue;
    const QString lowerSanitizedNewValue = sanitizedNewValue.toLower();
    for (const QString &knownBibTeXStringDelimiter : availableBibTeXStringDelimiters)
        if (knownBibTeXStringDelimiter.toLower() == lowerSanitizedNewValue) {
            sanitizedNewValue = knownBibTeXStringDelimiter;
            break;
        }
    if (!d->validateValueForBibTeXStringDelimiter(sanitizedNewValue)) return false;
    d->dirtyFlagBibTeXStringDelimiter = false;
    d->cachedBibTeXStringDelimiter = sanitizedNewValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXStringDelimiter"), Preferences::defaultBibTeXStringDelimiter);
    if (valueFromConfig == sanitizedNewValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXStringDelimiter"), sanitizedNewValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QVector<QPair<Preferences::CommentContext, QString>> Preferences::availableBibTeXCommentContexts {{Preferences::CommentContext::Verbatim, i18nc("Comment Context: The comment's text appears verbatim in the bibliography", "Verbatim")}, {Preferences::CommentContext::Prefix, i18nc("Comment Quoting: All lines of the comment share a common prefix like '%% ' which is not part of the comment's text", "Use prefix like '%% '")}, {Preferences::CommentContext::Command, i18nc("Comment Quoting: The comment is inside @comment{..}", "Use command @comment{...}")}};
const Preferences::CommentContext Preferences::defaultBibTeXCommentContext = Preferences::availableBibTeXCommentContexts.front().first;

Preferences::CommentContext Preferences::bibTeXCommentContext()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXCommentContext) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const Preferences::CommentContext valueFromConfig = static_cast<Preferences::CommentContext>(configGroup.readEntry(QStringLiteral("BibTeXCommentContext"), static_cast<int>(Preferences::defaultBibTeXCommentContext)));
        if (d->validateValueForBibTeXCommentContext(valueFromConfig)) {
            d->cachedBibTeXCommentContext = valueFromConfig;
            d->dirtyFlagBibTeXCommentContext = false;
        } else {
            /// Configuration file setting for BibTeXCommentContext has an invalid value, using default as fallback
            setBibTeXCommentContext(Preferences::defaultBibTeXCommentContext);
        }
    }
    return d->cachedBibTeXCommentContext;
#else // HAVE_KF
    return defaultBibTeXCommentContext;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXCommentContext(const Preferences::CommentContext newValue)
{
    if (!d->validateValueForBibTeXCommentContext(newValue)) return false;
    d->dirtyFlagBibTeXCommentContext = false;
    d->cachedBibTeXCommentContext = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const Preferences::CommentContext valueFromConfig = static_cast<Preferences::CommentContext>(configGroup.readEntry(QStringLiteral("BibTeXCommentContext"), static_cast<int>(Preferences::defaultBibTeXCommentContext)));
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXCommentContext"), static_cast<int>(newValue), KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QString Preferences::defaultBibTeXCommentPrefix = QString();

const QString &Preferences::bibTeXCommentPrefix()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXCommentPrefix) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXCommentPrefix"), Preferences::defaultBibTeXCommentPrefix);
        if (d->validateValueForBibTeXCommentPrefix(valueFromConfig)) {
            d->cachedBibTeXCommentPrefix = valueFromConfig;
            d->dirtyFlagBibTeXCommentPrefix = false;
        } else {
            /// Configuration file setting for BibTeXCommentPrefix has an invalid value, using default as fallback
            setBibTeXCommentPrefix(Preferences::defaultBibTeXCommentPrefix);
        }
    }
    return d->cachedBibTeXCommentPrefix;
#else // HAVE_KF
    return defaultBibTeXCommentPrefix;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXCommentPrefix(const QString &newValue)
{
    if (!d->validateValueForBibTeXCommentPrefix(newValue)) return false;
    d->dirtyFlagBibTeXCommentPrefix = false;
    d->cachedBibTeXCommentPrefix = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXCommentPrefix"), Preferences::defaultBibTeXCommentPrefix);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXCommentPrefix"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QVector<QPair<KBibTeX::Casing, QString>> Preferences::availableBibTeXKeywordCasings {{KBibTeX::Casing::LowerCase, i18nc("Casing of strings", "lowercase")}, {KBibTeX::Casing::InitialCapital, i18nc("Casing of strings", "Initial capital")}, {KBibTeX::Casing::UpperCamelCase, i18nc("Casing of strings", "UpperCamelCase")}, {KBibTeX::Casing::LowerCamelCase, i18nc("Casing of strings", "lowerCamelCase")}, {KBibTeX::Casing::UpperCase, i18nc("Casing of strings", "UPPERCASE")}};
const KBibTeX::Casing Preferences::defaultBibTeXKeywordCasing = Preferences::availableBibTeXKeywordCasings.front().first;

KBibTeX::Casing Preferences::bibTeXKeywordCasing()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXKeywordCasing) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const KBibTeX::Casing valueFromConfig = static_cast<KBibTeX::Casing>(configGroup.readEntry(QStringLiteral("BibTeXKeywordCasing"), static_cast<int>(Preferences::defaultBibTeXKeywordCasing)));
        if (d->validateValueForBibTeXKeywordCasing(valueFromConfig)) {
            d->cachedBibTeXKeywordCasing = valueFromConfig;
            d->dirtyFlagBibTeXKeywordCasing = false;
        } else {
            /// Configuration file setting for BibTeXKeywordCasing has an invalid value, using default as fallback
            setBibTeXKeywordCasing(Preferences::defaultBibTeXKeywordCasing);
        }
    }
    return d->cachedBibTeXKeywordCasing;
#else // HAVE_KF
    return defaultBibTeXKeywordCasing;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXKeywordCasing(const KBibTeX::Casing newValue)
{
    if (!d->validateValueForBibTeXKeywordCasing(newValue)) return false;
    d->dirtyFlagBibTeXKeywordCasing = false;
    d->cachedBibTeXKeywordCasing = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const KBibTeX::Casing valueFromConfig = static_cast<KBibTeX::Casing>(configGroup.readEntry(QStringLiteral("BibTeXKeywordCasing"), static_cast<int>(Preferences::defaultBibTeXKeywordCasing)));
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXKeywordCasing"), static_cast<int>(newValue), KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const bool Preferences::defaultBibTeXProtectCasing = true;

bool Preferences::bibTeXProtectCasing()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXProtectCasing) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const bool valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXProtectCasing"), Preferences::defaultBibTeXProtectCasing);
        if (d->validateValueForBibTeXProtectCasing(valueFromConfig)) {
            d->cachedBibTeXProtectCasing = valueFromConfig;
            d->dirtyFlagBibTeXProtectCasing = false;
        } else {
            /// Configuration file setting for BibTeXProtectCasing has an invalid value, using default as fallback
            setBibTeXProtectCasing(Preferences::defaultBibTeXProtectCasing);
        }
    }
    return d->cachedBibTeXProtectCasing;
#else // HAVE_KF
    return defaultBibTeXProtectCasing;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXProtectCasing(const bool newValue)
{
    if (!d->validateValueForBibTeXProtectCasing(newValue)) return false;
    d->dirtyFlagBibTeXProtectCasing = false;
    d->cachedBibTeXProtectCasing = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const bool valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXProtectCasing"), Preferences::defaultBibTeXProtectCasing);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXProtectCasing"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QStringList Preferences::availableBibTeXListSeparators {QStringLiteral("; "), QStringLiteral(", ")};
const QString Preferences::defaultBibTeXListSeparator = Preferences::availableBibTeXListSeparators.front();

const QString &Preferences::bibTeXListSeparator()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXListSeparator) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXListSeparator"), Preferences::defaultBibTeXListSeparator);
        if (d->validateValueForBibTeXListSeparator(valueFromConfig)) {
            d->cachedBibTeXListSeparator = valueFromConfig;
            d->dirtyFlagBibTeXListSeparator = false;
        } else {
            /// Configuration file setting for BibTeXListSeparator has an invalid value, using default as fallback
            setBibTeXListSeparator(Preferences::defaultBibTeXListSeparator);
        }
    }
    return d->cachedBibTeXListSeparator;
#else // HAVE_KF
    return defaultBibTeXListSeparator;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXListSeparator(const QString &newValue)
{
    QString sanitizedNewValue = newValue;
    const QString lowerSanitizedNewValue = sanitizedNewValue.toLower();
    for (const QString &knownBibTeXListSeparator : availableBibTeXListSeparators)
        if (knownBibTeXListSeparator.toLower() == lowerSanitizedNewValue) {
            sanitizedNewValue = knownBibTeXListSeparator;
            break;
        }
    if (!d->validateValueForBibTeXListSeparator(sanitizedNewValue)) return false;
    d->dirtyFlagBibTeXListSeparator = false;
    d->cachedBibTeXListSeparator = sanitizedNewValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXListSeparator"), Preferences::defaultBibTeXListSeparator);
    if (valueFromConfig == sanitizedNewValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXListSeparator"), sanitizedNewValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const bool Preferences::defaultbibTeXEntriesSortedByIdentifier = false;

bool Preferences::bibTeXEntriesSortedByIdentifier()
{
#ifdef HAVE_KF
    if (d->dirtyFlagbibTeXEntriesSortedByIdentifier) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
        const bool valueFromConfig = configGroup.readEntry(QStringLiteral("bibTeXEntriesSortedByIdentifier"), Preferences::defaultbibTeXEntriesSortedByIdentifier);
        if (d->validateValueForbibTeXEntriesSortedByIdentifier(valueFromConfig)) {
            d->cachedbibTeXEntriesSortedByIdentifier = valueFromConfig;
            d->dirtyFlagbibTeXEntriesSortedByIdentifier = false;
        } else {
            /// Configuration file setting for bibTeXEntriesSortedByIdentifier has an invalid value, using default as fallback
            setbibTeXEntriesSortedByIdentifier(Preferences::defaultbibTeXEntriesSortedByIdentifier);
        }
    }
    return d->cachedbibTeXEntriesSortedByIdentifier;
#else // HAVE_KF
    return defaultbibTeXEntriesSortedByIdentifier;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setbibTeXEntriesSortedByIdentifier(const bool newValue)
{
    if (!d->validateValueForbibTeXEntriesSortedByIdentifier(newValue)) return false;
    d->dirtyFlagbibTeXEntriesSortedByIdentifier = false;
    d->cachedbibTeXEntriesSortedByIdentifier = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterBibTeX"));
    const bool valueFromConfig = configGroup.readEntry(QStringLiteral("bibTeXEntriesSortedByIdentifier"), Preferences::defaultbibTeXEntriesSortedByIdentifier);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("bibTeXEntriesSortedByIdentifier"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QString Preferences::defaultLaTeXBabelLanguage = QStringLiteral("english");

const QString &Preferences::laTeXBabelLanguage()
{
#ifdef HAVE_KF
    if (d->dirtyFlagLaTeXBabelLanguage) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterLaTeXbased"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("LaTeXBabelLanguage"), Preferences::defaultLaTeXBabelLanguage);
        if (d->validateValueForLaTeXBabelLanguage(valueFromConfig)) {
            d->cachedLaTeXBabelLanguage = valueFromConfig;
            d->dirtyFlagLaTeXBabelLanguage = false;
        } else {
            /// Configuration file setting for LaTeXBabelLanguage has an invalid value, using default as fallback
            setLaTeXBabelLanguage(Preferences::defaultLaTeXBabelLanguage);
        }
    }
    return d->cachedLaTeXBabelLanguage;
#else // HAVE_KF
    return defaultLaTeXBabelLanguage;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setLaTeXBabelLanguage(const QString &newValue)
{
    if (!d->validateValueForLaTeXBabelLanguage(newValue)) return false;
    d->dirtyFlagLaTeXBabelLanguage = false;
    d->cachedLaTeXBabelLanguage = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterLaTeXbased"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("LaTeXBabelLanguage"), Preferences::defaultLaTeXBabelLanguage);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("LaTeXBabelLanguage"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QString Preferences::defaultBibTeXBibliographyStyle = QStringLiteral("plain");

const QString &Preferences::bibTeXBibliographyStyle()
{
#ifdef HAVE_KF
    if (d->dirtyFlagBibTeXBibliographyStyle) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("FileExporterLaTeXbased"));
        const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXBibliographyStyle"), Preferences::defaultBibTeXBibliographyStyle);
        if (d->validateValueForBibTeXBibliographyStyle(valueFromConfig)) {
            d->cachedBibTeXBibliographyStyle = valueFromConfig;
            d->dirtyFlagBibTeXBibliographyStyle = false;
        } else {
            /// Configuration file setting for BibTeXBibliographyStyle has an invalid value, using default as fallback
            setBibTeXBibliographyStyle(Preferences::defaultBibTeXBibliographyStyle);
        }
    }
    return d->cachedBibTeXBibliographyStyle;
#else // HAVE_KF
    return defaultBibTeXBibliographyStyle;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setBibTeXBibliographyStyle(const QString &newValue)
{
    if (!d->validateValueForBibTeXBibliographyStyle(newValue)) return false;
    d->dirtyFlagBibTeXBibliographyStyle = false;
    d->cachedBibTeXBibliographyStyle = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("FileExporterLaTeXbased"));
    const QString valueFromConfig = configGroup.readEntry(QStringLiteral("BibTeXBibliographyStyle"), Preferences::defaultBibTeXBibliographyStyle);
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("BibTeXBibliographyStyle"), newValue, KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QVector<QPair<Preferences::FileViewDoubleClickAction, QString>> Preferences::availableFileViewDoubleClickActions {{Preferences::FileViewDoubleClickAction::OpenEditor, i18nc("What to do if double-clicking on a file view item", "Open Editor")}, {Preferences::FileViewDoubleClickAction::ViewDocument, i18nc("What to do if double-clicking on a file view item", "View Document")}};
const Preferences::FileViewDoubleClickAction Preferences::defaultFileViewDoubleClickAction = Preferences::availableFileViewDoubleClickActions.front().first;

Preferences::FileViewDoubleClickAction Preferences::fileViewDoubleClickAction()
{
#ifdef HAVE_KF
    if (d->dirtyFlagFileViewDoubleClickAction) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("User Interface"));
        const Preferences::FileViewDoubleClickAction valueFromConfig = static_cast<Preferences::FileViewDoubleClickAction>(configGroup.readEntry(QStringLiteral("FileViewDoubleClickAction"), static_cast<int>(Preferences::defaultFileViewDoubleClickAction)));
        if (d->validateValueForFileViewDoubleClickAction(valueFromConfig)) {
            d->cachedFileViewDoubleClickAction = valueFromConfig;
            d->dirtyFlagFileViewDoubleClickAction = false;
        } else {
            /// Configuration file setting for FileViewDoubleClickAction has an invalid value, using default as fallback
            setFileViewDoubleClickAction(Preferences::defaultFileViewDoubleClickAction);
        }
    }
    return d->cachedFileViewDoubleClickAction;
#else // HAVE_KF
    return defaultFileViewDoubleClickAction;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setFileViewDoubleClickAction(const Preferences::FileViewDoubleClickAction newValue)
{
    if (!d->validateValueForFileViewDoubleClickAction(newValue)) return false;
    d->dirtyFlagFileViewDoubleClickAction = false;
    d->cachedFileViewDoubleClickAction = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("User Interface"));
    const Preferences::FileViewDoubleClickAction valueFromConfig = static_cast<Preferences::FileViewDoubleClickAction>(configGroup.readEntry(QStringLiteral("FileViewDoubleClickAction"), static_cast<int>(Preferences::defaultFileViewDoubleClickAction)));
    if (valueFromConfig == newValue) return false;
    configGroup.writeEntry(QStringLiteral("FileViewDoubleClickAction"), static_cast<int>(newValue), KConfig::Notify);
    d->config->sync();
    return true;
}
#endif // HAVE_KF

const QVector<QPair<QString, QString>> Preferences::defaultColorCodes {{QStringLiteral("#c30"), i18nc("Color Labels", "Important")}, {QStringLiteral("#03f"), i18nc("Color Labels", "Unread")}, {QStringLiteral("#096"), i18nc("Color Labels", "Read")}, {QStringLiteral("#fd0"), i18nc("Color Labels", "Watch")}};

const QVector<QPair<QString, QString>> &Preferences::colorCodes()
{
#ifdef HAVE_KF
    if (d->dirtyFlagColorCodes) {
        d->config->reparseConfiguration();
        static const KConfigGroup configGroup(d->config, QStringLiteral("Color Labels"));
        const QVector<QPair<QString, QString>> valueFromConfig = d->readEntryColorCodes(configGroup, QStringLiteral("ColorCodes"));
        if (d->validateValueForColorCodes(valueFromConfig)) {
            d->cachedColorCodes = valueFromConfig;
            d->dirtyFlagColorCodes = false;
        } else {
            /// Configuration file setting for ColorCodes has an invalid value, using default as fallback
            setColorCodes(Preferences::defaultColorCodes);
        }
    }
    return d->cachedColorCodes;
#else // HAVE_KF
    return defaultColorCodes;
#endif // HAVE_KF
}

#ifdef HAVE_KF
bool Preferences::setColorCodes(const QVector<QPair<QString, QString>> &newValue)
{
    if (!d->validateValueForColorCodes(newValue)) return false;
    d->dirtyFlagColorCodes = false;
    d->cachedColorCodes = newValue;
    static KConfigGroup configGroup(d->config, QStringLiteral("Color Labels"));
    const QVector<QPair<QString, QString>> valueFromConfig = d->readEntryColorCodes(configGroup, QStringLiteral("ColorCodes"));
    if (valueFromConfig == newValue) return false;
    d->writeEntryColorCodes(configGroup, QStringLiteral("ColorCodes"), newValue);
    d->config->sync();
    return true;
}
#endif // HAVE_KF
