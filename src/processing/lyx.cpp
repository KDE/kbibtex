/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <QWidget>
#include <QTreeView>
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

#include <kdeversion.h>

#include "lyx.h"

class LyX::LyXPrivate
{
private:
    LyX *p;

public:
    QTreeView *widget;
    KAction *action;
    QStringList references;
    KSharedConfigPtr config;
    const QString configGroupNameLyX;

    LyXPrivate(LyX *parent, QTreeView *widget)
            : p(parent), action(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupNameLyX(QLatin1String("LyX")) {
        this->widget = widget;
    }

    QString findLyXServerPipeName() {
        QString result = QString::null;

        QFile lyxConfigFile(QDir::homePath() + QDir::separator() + ".lyx" + QDir::separator() + "preferences"); // FIXME does this work on Windows/Mac systems?
        if (lyxConfigFile.open(QFile::ReadOnly)) {
            QRegExp serverPipeRegExp("^\\\\serverpipe\\s+\"([^\"]+)\"");
            QString line = QString::null;
            while (!(line = lyxConfigFile.readLine()).isNull()) {
                if (line.isEmpty() || line[0] == '#' || line.length() < 3) continue;
                if (serverPipeRegExp.indexIn(line) >= 0) {
                    result = serverPipeRegExp.cap(1) + ".in";
                    break;
                }
            }
            lyxConfigFile.close();
        }

        return result;
    }
};

const QString LyX::keyLyXServerPipeName = QLatin1String("LyXServerPipeName");
const QString LyX::defaultLyXServerPipeName = QLatin1String("");

LyX::LyX(KParts::ReadOnlyPart *part, QTreeView *widget)
        : QObject(part), d(new LyX::LyXPrivate(this, widget))
{
    d->action = new KAction(KIcon("application-x-lyx"), i18n("Send to LyX"), this);
    part->actionCollection()->addAction("sendtolyx", d->action);
    d->action->setEnabled(false);
    connect(d->action, SIGNAL(triggered()), this, SLOT(sendReferenceToLyX()));
#if KDE_VERSION_MINOR >= 4
    part->replaceXMLFile(KStandardDirs::locate("appdata", "lyx.rc"), KStandardDirs::locateLocal("appdata", "lyx.rc"), true);
#endif
    widget->addAction(d->action);
}

void LyX::setReferences(const QStringList &references)
{
    d->references = references;
}

void LyX::updateActions()
{
    d->action->setEnabled(d->widget != NULL && !d->widget->selectionModel()->selection().isEmpty());
}

void LyX::sendReferenceToLyX()
{
    const QString defaultHintOnLyXProblems = i18n("\n\nCheck that LyX is running and configured to receive references (see \"LyX server pipe\" in LyX's settings).");
    const QString msgBoxTitle = i18n("Send Reference to LyX");

    if (d->references.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No references to send to LyX."), msgBoxTitle);
        return;
    }

    KConfigGroup configGroup(d->config, d->configGroupNameLyX);
    QString pipeName = configGroup.readEntry(LyX::keyLyXServerPipeName, LyX::defaultLyXServerPipeName);
    if (pipeName.isEmpty()) {
        KMessageBox::error(d->widget, i18n("No \"LyX server pipe\" has been configured in KBibTeX's settings."), msgBoxTitle);
        return;
    }

    QFileInfo fi(pipeName);
    if (!fi.exists()) {
        KMessageBox::error(d->widget, i18n("LyX server pipe \"%1\" does not exist.", pipeName) + defaultHintOnLyXProblems, msgBoxTitle);
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
