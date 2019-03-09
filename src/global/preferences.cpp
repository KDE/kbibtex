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

#include "preferences.h"

#include <QDebug>
#include <QDir>

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigWatcher>
#include <KConfigGroup>
#else // HAVE_KF5
#define I18N_NOOP(text) QObject::tr(text)
#define i18n(text) QObject::tr(text)
#endif // HAVE_KF5

#ifdef HAVE_KF5
#include "notificationhub.h"
#endif // HAVE_KF5

const QString Preferences::groupColor = QStringLiteral("Color Labels");
const QString Preferences::keyColorCodes = QStringLiteral("colorCodes");
const QStringList Preferences::defaultColorCodes {QStringLiteral("#cc3300"), QStringLiteral("#0033ff"), QStringLiteral("#009966"), QStringLiteral("#f0d000")};
const QString Preferences::keyColorLabels = QStringLiteral("colorLabels");
// FIXME
// clazy warns: QString(const char*) being called [-Wclazy-qstring-uneeded-heap-allocations]
// ... but using QStringLiteral may break the translation process?
const QStringList Preferences::defaultColorLabels {I18N_NOOP("Important"), I18N_NOOP("Unread"), I18N_NOOP("Read"), I18N_NOOP("Watch")};

const QString Preferences::groupGeneral = QStringLiteral("General");

const QString Preferences::groupUserInterface = QStringLiteral("User Interface");
const QString Preferences::keyElementDoubleClickAction = QStringLiteral("elementDoubleClickAction");
const Preferences::ElementDoubleClickAction Preferences::defaultElementDoubleClickAction = ActionOpenEditor;

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

class Preferences::Private
{
public:
#ifdef HAVE_KF5
    KSharedConfigPtr config;
    KConfigWatcher::Ptr watcher;
#endif // HAVE_KF5

    Private(Preferences *)
    {
#ifdef HAVE_KF5
        config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));
        watcher = KConfigWatcher::create(config);
        dirtyFlagBibliographySystem = true;
        cachedBibliographySystem = defaultBibliographySystem;
        dirtyFlagPersonNameFormatting = true;
        cachedPersonNameFormatting = defaultPersonNameFormatting;
        dirtyFlagCopyReferenceCommand = true;
        cachedCopyReferenceCommand = defaultCopyReferenceCommand;
        dirtyFlagPageSize = true;
        cachedPageSize = defaultPageSize;
        dirtyFlagBackupScope = true;
        cachedBackupScope = defaultBackupScope;
        dirtyFlagNumberOfBackups = true;
        cachedNumberOfBackups = defaultNumberOfBackups;
        dirtyFlagIdSuggestionFormatStrings = true;
        cachedIdSuggestionFormatStrings = defaultIdSuggestionFormatStrings;
        dirtyFlagActiveIdSuggestionFormatString = true;
        cachedActiveIdSuggestionFormatString = defaultActiveIdSuggestionFormatString;
        dirtyFlagLyXUseAutomaticPipeDetection = true;
        cachedLyXUseAutomaticPipeDetection = defaultLyXUseAutomaticPipeDetection;
        dirtyFlagLyXPipePath = true;
        cachedLyXPipePath = defaultLyXPipePath;
#endif // HAVE_KF5
    }

#ifdef HAVE_KF5
    inline bool validateValueForBibliographySystem(const int valueToBeChecked) {
        for (QVector<QPair<Preferences::BibliographySystem, QString>>::ConstIterator it = Preferences::availableBibliographySystems.constBegin(); it != Preferences::availableBibliographySystems.constEnd(); ++it)
            if (static_cast<int>(it->first) == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForPersonNameFormatting(const QString &valueToBeChecked) {
        return valueToBeChecked.contains(QStringLiteral("%f")) && valueToBeChecked.contains(QStringLiteral("%l")) && valueToBeChecked.contains(QStringLiteral("%s"));
    }

    inline bool validateValueForPageSize(const int valueToBeChecked) {
        for (const auto &dbItem : Preferences::availablePageSizes)
            if (static_cast<int>(dbItem.internalPageSizeId) == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForCopyReferenceCommand(const QString &/*valueToBeChecked*/) {
        return true;
    }

    inline bool validateValueForBackupScope(const int valueToBeChecked) {
        for (const auto &dbItem : Preferences::availableBackupScopes)
            if (static_cast<int>(dbItem.first) == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForNumberOfBackups(const int valueToBeChecked) {
        return valueToBeChecked >= 0;
    }

    inline bool validateValueForIdSuggestionFormatStrings(const QStringList &valueToBeChecked) {
        return !valueToBeChecked.isEmpty() && (valueToBeChecked.front().contains(QLatin1Char('A')) || valueToBeChecked.front().contains(QLatin1Char('a')) || valueToBeChecked.front().contains(QLatin1Char('y')) || valueToBeChecked.front().contains(QLatin1Char('Y')) || valueToBeChecked.front().contains(QLatin1Char('T')));
    }

    inline bool validateValueForActiveIdSuggestionFormatString(const QString &) {
        return true;
    }

    inline bool validateValueForLyXUseAutomaticPipeDetection(const bool) {
        return true;
    }

    inline bool validateValueForLyXPipePath(const QString &valueToBeChecked) {
        return valueToBeChecked.startsWith(QLatin1Char('/'));
    }

    inline bool validateValueForBibTeXEncoding(const QString &valueToBeChecked) {
        return Preferences::availableBibTeXEncodings.contains(valueToBeChecked);
    }

#define getterSetter(type, typeInConfig, stem, notificationEventId, configGroupName) \
    bool dirtyFlag##stem; \
    type cached##stem; \
    bool set##stem(const type &newValue) { \
        dirtyFlag##stem = false; \
        cached##stem = newValue; \
        static KConfigGroup configGroup(config, QStringLiteral(configGroupName)); \
        const typeInConfig valueFromConfig = configGroup.readEntry(QStringLiteral(#stem), static_cast<typeInConfig>(Preferences::default##stem)); \
        const typeInConfig newValuePOD = static_cast<typeInConfig>(newValue); \
        if (valueFromConfig == newValuePOD) return false; \
        configGroup.writeEntry(QStringLiteral(#stem), newValuePOD, KConfig::Notify /** to catch changes via KConfigWatcher */); \
        config->sync(); \
        NotificationHub::publishEvent(notificationEventId); \
        return true; \
    } \
    type get##stem() { \
        if (dirtyFlag##stem) { \
            config->reparseConfiguration(); static const KConfigGroup configGroup(config, QStringLiteral(configGroupName)); \
            const typeInConfig valueFromConfiguration = configGroup.readEntry(QStringLiteral(#stem), static_cast<typeInConfig>(Preferences::default##stem)); \
            if (validateValueFor##stem(valueFromConfiguration)) { \
                cached##stem = static_cast<type>(valueFromConfiguration); \
                dirtyFlag##stem = false; \
            } else { \
                qWarning() << "Configuration file setting for" << #stem << "has an invalid value, using default as fallback"; \
                set##stem(Preferences::default##stem); \
            } \
        } \
        return cached##stem; \
    }

    getterSetter(Preferences::BibliographySystem, int, BibliographySystem, NotificationHub::EventBibliographySystemChanged, "General")
    getterSetter(QString, QString, PersonNameFormatting, NotificationHub::EventConfigurationChanged, "General")
    getterSetter(QPageSize::PageSizeId, int, PageSize, NotificationHub::EventConfigurationChanged, "General")
    getterSetter(QString, QString, CopyReferenceCommand, NotificationHub::EventConfigurationChanged, "LaTeX")
    getterSetter(Preferences::BackupScope, int, BackupScope, NotificationHub::EventConfigurationChanged, "InputOutput")
    getterSetter(int, int, NumberOfBackups, NotificationHub::EventConfigurationChanged, "InputOutput")
    getterSetter(QStringList, QStringList, IdSuggestionFormatStrings, NotificationHub::EventConfigurationChanged, "IdSuggestions")
    getterSetter(QString, QString, ActiveIdSuggestionFormatString, NotificationHub::EventConfigurationChanged, "IdSuggestions")
    getterSetter(bool, bool, LyXUseAutomaticPipeDetection, NotificationHub::EventConfigurationChanged, "LyX")
    getterSetter(QString, QString, LyXPipePath, NotificationHub::EventConfigurationChanged, "LyX")
    getterSetter(QString, QString, BibTeXEncoding, NotificationHub::EventConfigurationChanged, "FileExporterBibTeX")
#endif // HAVE_KF5
};

Preferences &Preferences::instance()
{
    static Preferences singleton;
    return singleton;
}

Preferences::Preferences()
        : d(new Preferences::Private(this))
{
#ifdef HAVE_KF5
    QObject::connect(d->watcher.data(), &KConfigWatcher::configChanged, [this](const KConfigGroup & group, const QByteArrayList & names) {
        QSet<int> eventsToPublish;
#define eventCheck(type, stem, notificationEventId, configGroupName) \
    if (group.name() == QStringLiteral(configGroupName) && names.contains(#stem)) { \
        qDebug() << "Configuration setting"<<#stem<<"got changed by another Preferences instance"; \
        d->dirtyFlag##stem = true; \
        eventsToPublish.insert(notificationEventId); \
    }
        eventCheck(Preferences::BibliographySystem, BibliographySystem, NotificationHub::EventBibliographySystemChanged, "General")
        eventCheck(QString, PersonNameFormatting, NotificationHub::EventConfigurationChanged, "General")
        eventCheck(QPageSize::PageSizeId, PageSize, NotificationHub::EventConfigurationChanged, "General")
        eventCheck(QString, CopyReferenceCommand, NotificationHub::EventConfigurationChanged, "LaTeX")
        eventCheck(Preferences::BackupScope, BackupScope, NotificationHub::EventConfigurationChanged, "InputOutput")
        eventCheck(int, NumberOfBackups, NotificationHub::EventConfigurationChanged, "InputOutput")

        for (const int eventId : eventsToPublish)
            NotificationHub::publishEvent(eventId);
    });
#endif // HAVE_KF5
}

Preferences::~Preferences()
{
    delete d;
}

const Preferences::BibliographySystem Preferences::defaultBibliographySystem = Preferences::BibTeX;

Preferences::BibliographySystem Preferences::bibliographySystem()
{
#ifdef HAVE_KF5
    return d->getBibliographySystem();
#else // HAVE_KF5
    return defaultBibliographySystem;
#endif // HAVE_KF5
}

bool Preferences::setBibliographySystem(const Preferences::BibliographySystem bibliographySystem)
{
#ifdef HAVE_KF5
    return d->setBibliographySystem(bibliographySystem);
#else // HAVE_KF5
    Q_UNUSED(bibliographySystem);
    return true;
#endif // HAVE_KF5
}

const QVector<QPair<Preferences::BibliographySystem, QString>> Preferences::availableBibliographySystems {{Preferences::BibTeX, i18n("BibTeX")}, {Preferences::BibLaTeX, i18n("BibLaTeX")}};


const QString Preferences::personNameFormatLastFirst = QStringLiteral("<%l><, %s><, %f>");
const QString Preferences::personNameFormatFirstLast = QStringLiteral("<%f ><%l>< %s>");
const QString Preferences::defaultPersonNameFormatting = Preferences::personNameFormatLastFirst;

QString Preferences::personNameFormatting()
{
#ifdef HAVE_KF5
    return d->getPersonNameFormatting();
#else // HAVE_KF5
    return defaultPersonNameFormatting;
#endif // HAVE_KF5
}

bool Preferences::setPersonNameFormatting(const QString &personNameFormatting)
{
#ifdef HAVE_KF5
    return d->setPersonNameFormatting(personNameFormatting);
#else // HAVE_KF5
    Q_UNUSED(personNameFormatting);
    return true;
#endif // HAVE_KF5
}


const QVector<QString> Preferences::availableCopyReferenceCommands {QStringLiteral("cite"), QStringLiteral("citealt"), QStringLiteral("citeauthor"), QStringLiteral("citeauthor*"), QStringLiteral("citeyear"), QStringLiteral("citeyearpar"), QStringLiteral("shortcite"), QStringLiteral("citet"), QStringLiteral("citet*"), QStringLiteral("citep"), QStringLiteral("citep*")}; // TODO more
const QString Preferences::defaultCopyReferenceCommand = availableCopyReferenceCommands.first();

QString Preferences::copyReferenceCommand()
{
#ifdef HAVE_KF5
    return d->getCopyReferenceCommand();
#else // HAVE_KF5
    return defaultCopyReferenceCommand;
#endif // HAVE_KF5
}

bool Preferences::setCopyReferenceCommand(const QString &copyReferenceCommand)
{
#ifdef HAVE_KF5
    return d->setCopyReferenceCommand(copyReferenceCommand);
#else // HAVE_KF5
    Q_UNUSED(copyReferenceCommand);
    return true;
#endif // HAVE_KF5
}


const QVector<Preferences::PageSizeDatabase> Preferences::availablePageSizes {{QPageSize::A4, QStringLiteral("a4paper")}, {QPageSize::Letter, QStringLiteral("letter")}, {QPageSize::Legal, QStringLiteral("legal")}};
const QPageSize::PageSizeId Preferences::defaultPageSize = Preferences::availablePageSizes.front().internalPageSizeId;

QPageSize::PageSizeId Preferences::pageSize()
{
#ifdef HAVE_KF5
    return d->getPageSize();
#else // HAVE_KF5
    return defaultPageSize;
#endif // HAVE_KF5
}

bool Preferences::setPageSize(const QPageSize::PageSizeId pageSizeId)
{
#ifdef HAVE_KF5
    return d->setPageSize(pageSizeId);
#else // HAVE_KF5
    Q_UNUSED(pageSizeId);
    return true;
#endif // HAVE_KF5
}

QString Preferences::pageSizeToLaTeXName(const QPageSize::PageSizeId pageSizeId)
{
    for (const auto &dbItem : availablePageSizes)
        if (dbItem.internalPageSizeId == pageSizeId)
            return dbItem.laTeXName;
    return QPageSize::name(pageSizeId).toLower(); ///< just a wild guess
}

const QVector<QPair<Preferences::BackupScope, QString>> Preferences::availableBackupScopes {{Preferences::NoBackup, i18n("No backups")}, {Preferences::LocalOnly, i18n("Local files only")}, {Preferences::BothLocalAndRemote, i18n("Both local and remote files")}};
const Preferences::BackupScope Preferences::defaultBackupScope = Preferences::availableBackupScopes.at(1).first;

Preferences::BackupScope Preferences::backupScope()
{
#ifdef HAVE_KF5
    return d->getBackupScope();
#else // HAVE_KF5
    return defaultBackupScope;
#endif // HAVE_KF5
}

bool Preferences::setBackupScope(const Preferences::BackupScope backupScope)
{
#ifdef HAVE_KF5
    return d->setBackupScope(backupScope);
#else // HAVE_KF5
    Q_UNUSED(backupScope);
    return true;
#endif // HAVE_KF5
}

const int Preferences::defaultNumberOfBackups = 5;

int Preferences::numberOfBackups()
{
#ifdef HAVE_KF5
    return d->getNumberOfBackups();
#else // HAVE_KF5
    return defaultNumberOfBackups;
#endif // HAVE_KF5
}

bool Preferences::setNumberOfBackups(const int numberOfBackups)
{
#ifdef HAVE_KF5
    return d->setNumberOfBackups(numberOfBackups);
#else // HAVE_KF5
    Q_UNUSED(numberOfBackups);
    return true;
#endif // HAVE_KF5
}


const QStringList Preferences::defaultIdSuggestionFormatStrings {QStringLiteral("A"), QStringLiteral("A2|y"), QStringLiteral("A3|y"), QStringLiteral("A4|y|\":|T5"), QStringLiteral("al|\":|T"), QStringLiteral("al|y"), QStringLiteral("al|Y"), QStringLiteral("Al\"-|\"-|y"), QStringLiteral("Al\"+|Y"), QStringLiteral("al|y|T"), QStringLiteral("al|Y|T3"), QStringLiteral("al|Y|T3l"), QStringLiteral("a|\":|Y|\":|T1"), QStringLiteral("a|y"), QStringLiteral("A|\":|Y")};

QStringList Preferences::idSuggestionFormatStrings()
{
#ifdef HAVE_KF5
    return d->getIdSuggestionFormatStrings();
#else // HAVE_KF5
    return defaultIdSuggestionFormatStrings;
#endif // HAVE_KF5
}

bool Preferences::setIdSuggestionFormatStrings(const QStringList &idSuggestionFormatStrings)
{
#ifdef HAVE_KF5
    return d->setIdSuggestionFormatStrings(idSuggestionFormatStrings);
#else // HAVE_KF5
    Q_UNUSED(idSuggestionFormatStrings);
    return true;
#endif // HAVE_KF5
}

const QString Preferences::defaultActiveIdSuggestionFormatString; ///< intentially empty

QString Preferences::activeIdSuggestionFormatString()
{
#ifdef HAVE_KF5
    return d->getActiveIdSuggestionFormatString();
#else // HAVE_KF5
    return defaultActiveIdSuggestionFormatString;
#endif // HAVE_KF5
}

bool Preferences::setActiveIdSuggestionFormatString(const QString &activeIdSuggestionFormatString)
{
#ifdef HAVE_KF5
    return d->setActiveIdSuggestionFormatString(activeIdSuggestionFormatString);
#else // HAVE_KF5
    Q_UNUSED(activeIdSuggestionFormatString);
    return true;
#endif // HAVE_KF5
}


const bool Preferences::defaultLyXUseAutomaticPipeDetection = true;

bool Preferences::lyXUseAutomaticPipeDetection()
{
#ifdef HAVE_KF5
    return d->getLyXUseAutomaticPipeDetection();
#else // HAVE_KF5
    return defaultLyXUseAutomaticPipeDetection;
#endif // HAVE_KF5
}

bool Preferences::setLyXUseAutomaticPipeDetection(const bool lyXUseAutomaticPipeDetection)
{
#ifdef HAVE_KF5
    return d->setLyXUseAutomaticPipeDetection(lyXUseAutomaticPipeDetection);
#else // HAVE_KF5
    Q_UNUSED(lyXUseAutomaticPipeDetection);
    return true;
#endif // HAVE_KF5
}

const QString Preferences::defaultLyXPipePath = QDir::homePath() + QStringLiteral("/.lyxpipe.in");

QString Preferences::lyXPipePath()
{
#ifdef HAVE_KF5
    return d->getLyXPipePath();
#else // HAVE_KF5
    return defaultLyXPipePath;
#endif // HAVE_KF5
}

bool Preferences::setLyXPipePath(const QString &lyXPipePath)
{
#ifdef HAVE_KF5
    QString sanitizedLyXPipePath = lyXPipePath;
    if (sanitizedLyXPipePath.endsWith(QStringLiteral(".out")))
        sanitizedLyXPipePath = sanitizedLyXPipePath.left(sanitizedLyXPipePath.length() - 4);
    if (!sanitizedLyXPipePath.endsWith(QStringLiteral(".in")))
        sanitizedLyXPipePath = sanitizedLyXPipePath.append(QStringLiteral(".in"));
    return d->setLyXPipePath(sanitizedLyXPipePath);
#else // HAVE_KF5
    Q_UNUSED(lyXPipePath);
    return true;
#endif // HAVE_KF5
}


const QString Preferences::defaultBibTeXEncoding = QStringLiteral("UTF-8");

QString Preferences::bibTeXEncoding()
{
#ifdef HAVE_KF5
    return d->getBibTeXEncoding();
#else // HAVE_KF5
    return defaultBibTeXEncoding;
#endif // HAVE_KF5
}

bool Preferences::setBibTeXEncoding(const QString &bibTeXEncoding)
{
#ifdef HAVE_KF5
    QString sanitizedBibTeXEncoding = bibTeXEncoding;
    const QString sanitizedBibTeXEncodingLower = sanitizedBibTeXEncoding.toLower();
    for (const QString &knownBibTeXEncoding : availableBibTeXEncodings)
        if (knownBibTeXEncoding.toLower() == sanitizedBibTeXEncodingLower) {
            sanitizedBibTeXEncoding = knownBibTeXEncoding;
            break;
        }
    return d->setBibTeXEncoding(sanitizedBibTeXEncoding);
#else // HAVE_KF5
    Q_UNUSED(bibTeXEncoding);
    return true;
#endif // HAVE_KF5
}

const QStringList &Preferences::availableBibTeXEncodings {
    /// use LaTeX commands for everything that is not ASCII
    QStringLiteral("LaTeX"),
    /// the classics
    QStringLiteral("US-ASCII"), /** effectively like 'LaTeX' encoding */
    /// ISO 8859 a.k.a. Latin codecs
    QStringLiteral("ISO-8859-1"), QStringLiteral("ISO-8859-2"), QStringLiteral("ISO-8859-3"), QStringLiteral("ISO-8859-4"), QStringLiteral("ISO-8859-5"), QStringLiteral("ISO-8859-6"), QStringLiteral("ISO-8859-7"), QStringLiteral("ISO-8859-8"), QStringLiteral("ISO-8859-9"), QStringLiteral("ISO-8859-10"), QStringLiteral("ISO-8859-13"), QStringLiteral("ISO-8859-14"), QStringLiteral("ISO-8859-15"), QStringLiteral("ISO-8859-16"),
    /// various Unicode codecs
    QStringLiteral("UTF-8"), QStringLiteral("UTF-16"), QStringLiteral("UTF-16BE"), QStringLiteral("UTF-16LE"), QStringLiteral("UTF-32"), QStringLiteral("UTF-32BE"), QStringLiteral("UTF-32LE"),
    /// various Cyrillic codecs
    QStringLiteral("KOI8-R"), QStringLiteral("KOI8-U"),
    /// various CJK codecs
    QStringLiteral("Big5"), QStringLiteral("Big5-HKSCS"), QStringLiteral("GB18030"), QStringLiteral("EUC-JP"), QStringLiteral("EUC-KR"), QStringLiteral("ISO 2022-JP"), QStringLiteral("Shift-JIS"),
    /// Windows codecs
    QStringLiteral("Windows-1250"), QStringLiteral("Windows-1251"), QStringLiteral("Windows-1252"), QStringLiteral("Windows-1253"), QStringLiteral("Windows-1254"), QStringLiteral("Windows-1255"), QStringLiteral("Windows-1256"), QStringLiteral("Windows-1257"), QStringLiteral("Windows-1258")
};
