/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2017 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#ifndef KBIBTEX_PART_PARTFACTORY_H
#define KBIBTEX_PART_PARTFACTORY_H

#include <KPluginFactory>

class KBibTeXPartFactory : public KPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "kbibtexpart.json")
    Q_INTERFACES(KPluginFactory)

public:
    KBibTeXPartFactory();
    ~KBibTeXPartFactory() override;

protected:

    /**
     * From KPluginFactory's documentation: "You may reimplement it to
     * provide a very flexible factory. This is especially useful to
     * provide generic factories for plugins implemeted using a scripting
     * language."
     */
    QObject *create(const char *iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString &keyword) override;

private:
    class Private;
    Private *const d;
};

#endif // KBIBTEX_PART_PARTFACTORY_H
