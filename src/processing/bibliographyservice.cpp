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

#include "bibliographyservice.h"

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocale>
#include <KMessageBox>

class BibliographyService::Private
{
private:
    // UNUSED BibliographyService *p;

    /// Representing configuration file "mimeapps.list"
    /// see http://www.freedesktop.org/wiki/Specifications/mime-actions-spec/
    KSharedConfig::Ptr configXDGMimeAppsList;
    /// Groups inside "mimeapps.list"
    KConfigGroup configGroupAddedKDEServiceAssociations, configGroupRemovedKDEServiceAssociations;
    KConfigGroup configGroupAddedAssociations, configGroupRemovedAssociations;

    /// Names of .desktop files for KBibTeX (application and KPart)
    static const QString kbibtexApplicationDesktop;
    static const QString kbibtexPartDesktop;
    /// Names of .desktop files for Kate (application and KPart)
    static const QString kateApplicationDesktop;
    static const QString katePartDesktop;

public:
    QWidget *parentWidget;
    const QStringList textBasedMimeTypes;

    Private(QWidget *w, BibliographyService */* UNUSED parent*/)
        : // UNUSED p(parent),
        configXDGMimeAppsList(KSharedConfig::openConfig(QLatin1String("mimeapps.list"), KConfig::NoGlobals, "xdgdata-apps")),
        configGroupAddedKDEServiceAssociations(configXDGMimeAppsList, "Added KDE Service Associations"),
        configGroupRemovedKDEServiceAssociations(configXDGMimeAppsList, "Removed KDE Service Associations"),
        configGroupAddedAssociations(configXDGMimeAppsList, "Added Associations"),
        configGroupRemovedAssociations(configXDGMimeAppsList, "Removed Associations"),
        parentWidget(w),
        textBasedMimeTypes(QStringList()
                           << QLatin1String("text/x-bibtex") ///< classical BibTeX bibliographies
                           << QLatin1String("application/x-research-info-systems") ///< Research Information Systems (RIS) bibliographies
                           << QLatin1String("application/x-isi-export-format")) ///< Information Sciences Institute (ISI) bibliographies
    {
        /// nothing
    }

    bool setKBibTeXforMimeType(const QString &mimetype, const bool isPlainTextFormat) {
        /// Check that configuration file is writeable before continuing
        if (!configXDGMimeAppsList->isConfigWritable(true))
            return false;

        /// Configuration which application to use to open bibliography files
        QStringList addedAssociations = configGroupAddedAssociations.readXdgListEntry(mimetype, QStringList());
        /// Remove KBibTeX from list (will be added later to list's head)
        addedAssociations.removeAll(kbibtexApplicationDesktop);
        if (isPlainTextFormat) {
            /// Remove Kate from list (will be added later to list's second position)
            addedAssociations.removeAll(kateApplicationDesktop);
            /// Add Kate to list's head (will turn up as second)
            addedAssociations.prepend(kateApplicationDesktop);
        }
        /// Add KBibTeX to list's head
        addedAssociations.prepend(kbibtexApplicationDesktop);
        /// Write out and sync changes
        configGroupAddedAssociations.writeXdgListEntry(mimetype, addedAssociations);
        configGroupAddedAssociations.sync();

        /// Configuration which part to use to open bibliography files
        QStringList addedKDEServiceAssociations = configGroupAddedKDEServiceAssociations.readXdgListEntry(mimetype, QStringList());
        /// Remove KBibTeX from list (will be added later to list's head)
        addedKDEServiceAssociations.removeAll(kbibtexPartDesktop);
        if (isPlainTextFormat) {
            /// Remove Kate from list (will be added later to list's second position)
            addedKDEServiceAssociations.removeAll(katePartDesktop);
            /// Add Kate to list's head (will turn up as second)
            addedKDEServiceAssociations.prepend(katePartDesktop);
        }
        /// Add KBibTeX to list's head
        addedKDEServiceAssociations.prepend(kbibtexPartDesktop);
        /// Write out and sync changes
        configGroupAddedKDEServiceAssociations.writeXdgListEntry(mimetype, addedKDEServiceAssociations);
        configGroupAddedKDEServiceAssociations.sync();

        /// Configuration which application NOT to use to open bibliography files
        QStringList removedAssociations = configGroupRemovedAssociations.readXdgListEntry(mimetype, QStringList());
        /// If list of applications not to use is not empty ...
        if (!removedAssociations.isEmpty()) {
            /// Remove KBibTeX from list
            removedAssociations.removeAll(kbibtexApplicationDesktop);
            if (isPlainTextFormat)
                /// Remove Kate from list
                removedAssociations.removeAll(kateApplicationDesktop);
            if (removedAssociations.isEmpty())
                /// Empty lists can be removed from configuration file
                configGroupRemovedAssociations.deleteEntry(mimetype);
            else
                /// Write out updated list
                configGroupRemovedAssociations.writeXdgListEntry(mimetype, removedAssociations);
            /// Sync changes
            configGroupRemovedAssociations.sync();
        }

        /// Configuration which part NOT to use to open bibliography files
        QStringList removedKDEServiceAssociations = configGroupRemovedKDEServiceAssociations.readXdgListEntry(mimetype, QStringList());
        /// If list of parts not to use is not empty ...
        if (!removedKDEServiceAssociations.isEmpty()) {
            /// Remove KBibTeX part from list
            removedKDEServiceAssociations.removeAll(kbibtexPartDesktop);
            if (isPlainTextFormat)
                /// Remove Kate part from list
                removedAssociations.removeAll(katePartDesktop);
            if (removedKDEServiceAssociations.isEmpty())
                /// Empty lists can be removed from configuration file
                configGroupRemovedKDEServiceAssociations.deleteEntry(mimetype);
            else
                /// Write out updated list
                configGroupRemovedKDEServiceAssociations.writeXdgListEntry(mimetype, removedKDEServiceAssociations);
            /// Sync changes
            configGroupRemovedKDEServiceAssociations.sync();
        }

        return true;
    }

    bool isKBibTeXdefaultForMimeType(const QString &mimetype) const {
        /// Fetch all four configuration groups
        const QStringList addedAssociations = configGroupAddedAssociations.readXdgListEntry(mimetype, QStringList());
        const QStringList addedKDEServiceAssociations = configGroupAddedKDEServiceAssociations.readXdgListEntry(mimetype, QStringList());
        const QStringList removedAssociations = configGroupRemovedAssociations.readXdgListEntry(mimetype, QStringList());
        const QStringList removedKDEServiceAssociations = configGroupRemovedKDEServiceAssociations.readXdgListEntry(mimetype, QStringList());
        /// KBibTeX is default editor for bibliography if ...
        /// - the list of applications associated to mime type is not empty
        /// - KBibTeX is head of this list
        /// - KBibTeX is not named in the list of applications not be used
        /// - the list of parts associated to mime type is not empty
        /// - KBibTeX part is head of this list
        /// - KBibTeX part is not named in the list of parts not be used
        return !addedAssociations.isEmpty() && addedAssociations.first() == kbibtexApplicationDesktop && !removedAssociations.contains(kbibtexApplicationDesktop) && !addedKDEServiceAssociations.isEmpty() && addedKDEServiceAssociations.first() == kbibtexPartDesktop && !removedKDEServiceAssociations.contains(kbibtexPartDesktop);
    }
};

const QString BibliographyService::Private::kbibtexApplicationDesktop = QLatin1String("kde4-kbibtex.desktop");
const QString BibliographyService::Private::kbibtexPartDesktop = QLatin1String("kbibtexpart.desktop");
const QString BibliographyService::Private::kateApplicationDesktop = QLatin1String("kde4-kate.desktop");
const QString BibliographyService::Private::katePartDesktop = QLatin1String("katepart.desktop");

BibliographyService::BibliographyService(QWidget *parentWidget)
        : QObject(parentWidget), d(new BibliographyService::Private(parentWidget, this))
{
    /// nothing
}

void BibliographyService::setKBibTeXasDefault() {
    /// Go through all supported mime types
    foreach(const QString &mimeType, d->textBasedMimeTypes) {
        d->setKBibTeXforMimeType(mimeType, true);
    }

    /// kbuildsycoca4 has to be run to update the mime type associations
    QProcess *kbuildsycoca4Process = new QProcess(d->parentWidget);
    connect(kbuildsycoca4Process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(kbuildsycoca4finished(int,QProcess::ExitStatus)));
    kbuildsycoca4Process->start(QLatin1String("kbuildsycoca4"));
}

bool BibliographyService::isKBibTeXdefault() const {
    /// Go through all supported mime types
    foreach(const QString &mimeType, d->textBasedMimeTypes) {
        /// Test if KBibTeX is default handler for mime type
        if (!d->isKBibTeXdefaultForMimeType(mimeType))
            return false; ///< Failing any test means KBibTeX is not default application/part
    }
    return true; ///< All tests passed, KBibTeX is default application/part
}

void BibliographyService::kbuildsycoca4finished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitCode != 0 || exitStatus != QProcess::NormalExit)
        KMessageBox::error(d->parentWidget, i18n("Failed to run 'kbuildsycoca4' to update mime type associations.\n\nThe system may not know how to use KBibTeX to open bibliography files."), i18n("Failed to run 'kbuildsycoca4'"));
}
