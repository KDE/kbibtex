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

#include "lyx.h"

#include <QWidget>
#include <QAction>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QStandardPaths>
#include <qplatformdefs.h>

#include <KActionCollection>
#include <KLocalizedString>
#include <KParts/ReadOnlyPart>
#include <KMessageBox>
#include <KSharedConfig>
#include <KConfigGroup>

class LyX::LyXPrivate
{
public:
    QWidget *widget;
    QAction *action;
    QStringList references;

    KSharedConfigPtr config;
    const KConfigGroup group;

    LyXPrivate(LyX *parent, QWidget *widget)
            : action(nullptr), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), group(config, LyX::configGroupName) {
        Q_UNUSED(parent)
        this->widget = widget;
    }

    QString locateConfiguredLyXPipe() {
        QString result;

        /// First, check if automatic detection is disabled.
        /// In this case, read the LyX pipe's path from configuration
        if (!group.readEntry(keyUseAutomaticLyXPipeDetection, defaultUseAutomaticLyXPipeDetection))
            result = group.readEntry(keyLyXPipePath, defaultLyXPipePath);

#ifdef QT_LSTAT
        /// Check if the result so far is empty. This means that
        /// either automatic detection is enabled or the path in
        /// the configuration is empty/invalid. Proceed with
        /// automatic detection in this case.
        if (result.isEmpty())
            result = LyX::guessLyXPipeLocation();
#endif // QT_LSTAT

        /// Finally, even if automatic detection was preferred by the user,
        /// still check configuration for a path if automatic detection failed
        if (result.isEmpty() && group.readEntry(keyUseAutomaticLyXPipeDetection, defaultUseAutomaticLyXPipeDetection))
            result = group.readEntry(keyLyXPipePath, defaultLyXPipePath);

        /// Return the best found LyX pipe path
        return result;
    }
};

const QString LyX::keyUseAutomaticLyXPipeDetection = QStringLiteral("UseAutomaticLyXPipeDetection");
const QString LyX::keyLyXPipePath = QStringLiteral("LyXPipePath");
const bool LyX::defaultUseAutomaticLyXPipeDetection = true;
const QString LyX::defaultLyXPipePath = QString();
const QString LyX::configGroupName = QStringLiteral("LyXPipe");


LyX::LyX(KParts::ReadOnlyPart *part, QWidget *widget)
        : QObject(part), d(new LyX::LyXPrivate(this, widget))
{
    d->action = new QAction(QIcon::fromTheme(QStringLiteral("application-x-lyx")), i18n("Send to LyX/Kile"), this);
    part->actionCollection()->addAction(QStringLiteral("sendtolyx"), d->action);
    d->action->setEnabled(false);
    connect(d->action, &QAction::triggered, this, &LyX::sendReferenceToLyX);
    widget->addAction(d->action);
}

LyX::~LyX()
{
    delete d;
}

void LyX::setReferences(const QStringList &references)
{
    d->references = references;
    d->action->setEnabled(d->widget != nullptr && !d->references.isEmpty());
}

void LyX::sendReferenceToLyX()
{
    const QString defaultHintOnLyXProblems = i18n("\n\nCheck that LyX or Kile are running and configured to receive references.");
    const QString msgBoxTitle = i18n("Send Reference to LyX");
    /// LyX pipe name has to determined always fresh in case LyX or Kile exited
    const QString pipeName = d->locateConfiguredLyXPipe();

    if (pipeName.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No 'LyX server pipe' was detected.") + defaultHintOnLyXProblems, msgBoxTitle);
        return;
    }

    if (d->references.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No references to send to LyX/Kile."), msgBoxTitle);
        return;
    }

    QFile pipe(pipeName);
    if (!QFileInfo::exists(pipeName) || !pipe.open(QFile::WriteOnly)) {
        KMessageBox::error(d->widget, i18n("Could not open LyX server pipe '%1'.", pipeName) + defaultHintOnLyXProblems, msgBoxTitle);
        return;
    }

    QTextStream ts(&pipe);
    QString msg = QString(QStringLiteral("LYXCMD:kbibtex:citation-insert:%1")).arg(d->references.join(QStringLiteral(",")));

    ts << msg << endl;
    ts.flush();

    pipe.close();
}

#ifdef QT_LSTAT
QString LyX::guessLyXPipeLocation()
{
    QT_STATBUF statBuffer;
    const QStringList nameFilter = QStringList() << QStringLiteral("*lyxpipe*in*");
    QString result;

    /// Start with scanning the user's home directory for pipes
    QDir home = QDir::home();
    const QStringList files = home.entryList(nameFilter, QDir::Hidden | QDir::System | QDir::Writable, QDir::Unsorted);
    for (const QString &filename : files) {
        QString const absoluteFilename = home.absolutePath() + QDir::separator() + filename;
        if (QT_LSTAT(absoluteFilename.toLatin1(), &statBuffer) == 0 && S_ISFIFO(statBuffer.st_mode)) {
            result = absoluteFilename;
            break;
        }
    }

    /// No hit yet? Search LyX's configuration directory
    if (result.isEmpty()) {
        QDir home = QDir::home();
        if (home.cd(QStringLiteral(".lyx"))) {
            /// Same search again here
            const QStringList files = home.entryList(nameFilter, QDir::Hidden | QDir::System | QDir::Writable, QDir::Unsorted);
            for (const QString &filename : files) {
                QString const absoluteFilename = home.absolutePath() + QDir::separator() + filename;
                if (QT_LSTAT(absoluteFilename.toLatin1(), &statBuffer) == 0 && S_ISFIFO(statBuffer.st_mode)) {
                    result = absoluteFilename;
                    break;
                }
            }
        }
    }

    return result;
}
#endif // QT_LSTAT
