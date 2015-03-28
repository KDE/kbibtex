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

#include "partfactory.h"

#include <QDebug>

#include <KLocalizedString>

#include "part.h"
#include "version.h"

KBibTeXPartFactory::KBibTeXPartFactory()
        : KPluginFactory(), m_aboutData(QStringLiteral("kbibtexpart"), i18n("KBibTeXPart"), versionNumber, i18n("BibTeX Editor Component"), KAboutLicense::GPL_V2, i18n("Copyright 2004-2015 Thomas Fischer"))
{
    m_aboutData.addAuthor(i18n("Thomas Fischer"), i18n("Maintainer"), QLatin1String("fischer@unix-ag.uni-kl.de"));
    qDebug() << "Creating KBibTeXPart version" << versionNumber;
}


KBibTeXPartFactory::~KBibTeXPartFactory()
{
    /// nothing
}

QObject* KBibTeXPartFactory::create(const char *cn, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString &keyword)
// KParts::Part *KBibTeXPartFactory::createPartObject(QWidget *parentWidget, QObject *parent, const char *cn, const QStringList &/*args*/)
{
    Q_UNUSED(args)
    Q_UNUSED(keyword);

    const QByteArray classname(cn);
    bool readOnlyWanted = (classname == "Browser/View") || (classname == "KParts::ReadOnlyPart");

    KBibTeXPart *part = new KBibTeXPart(parentWidget, parent, m_aboutData);
    part->setReadWrite(!readOnlyWanted);

    return part;
}
