/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
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
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <sys/stat.h>

#include <QWidget>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KParts/ReadOnlyPart>
#include <KMessageBox>
#include <KStandardDirs>
#include <KDebug>

#include <kdeversion.h>

#include "lyx.h"

class LyX::LyXPrivate
{
private:
    LyX *p;

public:
    QWidget *widget;
    KAction *action;
    QStringList references;
    KSharedConfigPtr config;
    const QString configGroupNameLyX;

    LyXPrivate(LyX *parent, QWidget *widget)
            : p(parent), action(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupNameLyX(QLatin1String("LyX")) {
        this->widget = widget;
    }
};

LyX::LyX(KParts::ReadOnlyPart *part, QWidget *widget)
        : QObject(part), d(new LyX::LyXPrivate(this, widget))
{
    d->action = new KAction(KIcon("application-x-lyx"), i18n("Send Reference to LyX"), this);
    part->actionCollection()->addAction("sendtolyx", d->action);
    d->action->setEnabled(false);
    connect(d->action, SIGNAL(triggered()), this, SLOT(sendReferenceToLyX()));
#if KDE_VERSION_MINOR >= 4
    part->replaceXMLFile(KStandardDirs::locate("data", "kbibtex/lyx.rc"), KStandardDirs::locateLocal("data", "kbibtex/lyx.rc"), true);
#endif
    widget->addAction(d->action);
}

void LyX::setReferences(const QStringList &references)
{
    d->references = references;
    d->action->setEnabled(d->widget != NULL && !d->references.isEmpty());
}

void LyX::sendReferenceToLyX()
{
    const QString defaultHintOnLyXProblems = i18n("\n\nCheck that LyX or Kile are running and configured to receive references.");
    const QString msgBoxTitle = i18n("Send Reference to LyX");
    /// LyX pipe name has to determined always fresh in case LyX or Kile exited
    const QString pipeName = findLyXPipe();

    if (pipeName.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No \"LyX server pipe\" was detected.") + defaultHintOnLyXProblems, msgBoxTitle);
        return;
    }

    if (d->references.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No references to send to LyX."), msgBoxTitle);
        return;
    }

    QFile pipe(pipeName);
    if (!pipe.open(QFile::WriteOnly)) {
        KMessageBox::error(d->widget, i18n("Could not open LyX server pipe \"%1\".", pipeName) + defaultHintOnLyXProblems, msgBoxTitle);
        return;
    }

    QTextStream ts(&pipe);
    QString msg = QString("LYXCMD:kbibtex:citation-insert:%1").arg(d->references.join(","));

    ts << msg << endl;
    ts.flush();

    pipe.close();
}

QString LyX::findLyXPipe()
{
    // FIXME: This is most likely Unix/Linux specific. Works on Windows?

    struct stat fileInfo;
    const QStringList nameFilter = QStringList() << QLatin1String("*lyxpipe*in*");

    /// Start with scanning the user's home directory for pipes
    QDir home = QDir::home();
    QStringList files = home.entryList(nameFilter, QDir::Hidden | QDir::System | QDir::Writable, QDir::Unsorted);
    foreach(const QString &filename, files) {
        if (stat(filename.toAscii(), &fileInfo) == 0 && S_ISFIFO(fileInfo.st_mode))
            return home.absolutePath() + QDir::separator() + filename;
    }

    /// No hit yet? Search LyX's configuration directory
    if (home.cd(QLatin1String(".lyx"))) {
        /// Same search again here
        files = home.entryList(nameFilter, QDir::Hidden | QDir::System | QDir::Writable, QDir::Unsorted);
        foreach(const QString &filename, files) {
            if (stat(filename.toAscii(), &fileInfo) == 0 && S_ISFIFO(fileInfo.st_mode))
                return home.absolutePath() + QDir::separator() + filename;
        }
    }

    /// Obviously, nothing has been found, so return a Null string
    return QString::null;
}
