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

class Preferences::Private
{
private:
    Preferences *parent;

public:
#ifdef HAVE_KF5
    KSharedConfigPtr config;
    KConfigWatcher::Ptr watcher;
#endif // HAVE_KF5

    Private(Preferences *_parent)
            : parent(_parent)
    {
#ifdef HAVE_KF5
        config = KSharedConfig::openConfig(QStringLiteral("kbibtexrc"));
        watcher = KConfigWatcher::create(config);
        dirtyFlagBibliographySystem = true;
        cachedBibliographySystem = defaultBibliographySystem;
        dirtyFlagPersonNameFormatting = true;
        cachedPersonNameFormatting = defaultPersonNameFormatting;
#endif // HAVE_KF5
    }

    inline bool validateValueForBibliographySystem(const int valueToBeChecked) {
        for (QVector<QPair<Preferences::BibliographySystem, QString>>::ConstIterator it = Preferences::availableBibliographySystems.constBegin(); it != Preferences::availableBibliographySystems.constEnd(); ++it)
            if (static_cast<int>(it->first) == valueToBeChecked) return true;
        return false;
    }

    inline bool validateValueForPersonNameFormatting(const QString &valueToBeChecked) {
        return valueToBeChecked.contains(QStringLiteral("%f")) && valueToBeChecked.contains(QStringLiteral("%l")) && valueToBeChecked.contains(QStringLiteral("%s"));
    }

#ifdef HAVE_KF5
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
#endif // HAVE_KF5
    return true;
}
