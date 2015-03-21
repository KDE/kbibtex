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

#include "entrylayout.h"

#include <QDebug>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KStandardDirs>
#include <KLocale>

static const int entryLayoutMaxTabCount = 256;
static const int entryLayoutMaxFieldPerTabCount = 256;

class EntryLayout::EntryLayoutPrivate
{
public:
    EntryLayout *p;

    KSharedConfigPtr layoutConfig;

    static EntryLayout *singleton;

    EntryLayoutPrivate(EntryLayout *parent)
            : p(parent) {
        KSharedConfigPtr config(KSharedConfig::openConfig("kbibtexrc"));
        KConfigGroup configGroup(config, QString("User Interface"));
        const QString stylefile = configGroup.readEntry("CurrentStyle", "bibtex").append(".kbstyle").prepend("kbibtex/");
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, "data");
    }

    static QString convert(KBibTeX::FieldInputType fil) {
        switch (fil) {
        case KBibTeX::SingleLine : return QLatin1String("SingleLine");
        case KBibTeX::MultiLine : return QLatin1String("MultiLine");
        case KBibTeX::List : return QLatin1String("List");
        case KBibTeX::URL : return QLatin1String("URL");
        case KBibTeX::Month : return QLatin1String("Month");
        case KBibTeX::Color : return QLatin1String("Color");
        case KBibTeX::PersonList : return QLatin1String("PersonList");
        case KBibTeX::KeywordList : return QLatin1String("KeywordList");
        case KBibTeX::CrossRef : return QLatin1String("CrossRef");
        case KBibTeX::StarRating : return QLatin1String("StarRating");
        case KBibTeX::UrlList : return QLatin1String("UrlList");
        }
        return QString();
    }

    static KBibTeX::FieldInputType convert(const QString &text) {
        if (text == QLatin1String("List"))
            return KBibTeX::List;
        else if (text == QLatin1String("MultiLine"))
            return KBibTeX::MultiLine;
        else if (text == QLatin1String("URL"))
            return KBibTeX::URL;
        else if (text == QLatin1String("UrlList"))
            return KBibTeX::UrlList;
        else if (text == QLatin1String("Month"))
            return KBibTeX::Month;
        else if (text == QLatin1String("Color"))
            return KBibTeX::Color;
        else if (text == QLatin1String("PersonList"))
            return KBibTeX::PersonList;
        else if (text == QLatin1String("KeywordList"))
            return KBibTeX::KeywordList;
        else if (text == QLatin1String("CrossRef"))
            return KBibTeX::CrossRef;
        else if (text == QLatin1String("StarRating"))
            return KBibTeX::StarRating;
        else
            return KBibTeX::SingleLine;
    }
};

EntryLayout *EntryLayout::EntryLayoutPrivate::singleton = NULL;

EntryLayout::EntryLayout()
        : QVector<QSharedPointer<EntryTabLayout> >(), d(new EntryLayoutPrivate(this))
{
    load();
}

EntryLayout::~EntryLayout()
{
    delete d;
}

EntryLayout *EntryLayout::self()
{
    if (EntryLayoutPrivate::singleton == NULL)
        EntryLayoutPrivate::singleton  = new EntryLayout();
    return EntryLayoutPrivate::singleton;
}

void EntryLayout::load()
{
    clear();

    QString groupName = QLatin1String("EntryLayoutTab");
    KConfigGroup configGroup(d->layoutConfig, groupName);
    int tabCount = qMin(configGroup.readEntry("count", 0), entryLayoutMaxTabCount);

    for (int tab = 1; tab <= tabCount; ++tab) {
        QString groupName = QString("EntryLayoutTab%1").arg(tab);
        KConfigGroup configGroup(d->layoutConfig, groupName);

        QSharedPointer<EntryTabLayout> etl = QSharedPointer<EntryTabLayout>(new EntryTabLayout);
        etl->uiCaption = i18n(configGroup.readEntry("uiCaption", QString()).toUtf8().constData());
        etl->iconName = configGroup.readEntry("iconName", "entry");
        etl->columns = configGroup.readEntry("columns", 1);
        if (etl->uiCaption.isEmpty())
            continue;

        int fieldCount = qMin(configGroup.readEntry("count", 0), entryLayoutMaxFieldPerTabCount);
        for (int field = 1; field <= fieldCount; ++field) {
            SingleFieldLayout sfl;
            sfl.bibtexLabel = configGroup.readEntry(QString("bibtexLabel%1").arg(field), QString());
            sfl.uiLabel = i18n(configGroup.readEntry(QString("uiLabel%1").arg(field), QString()).toUtf8().constData());
            sfl.fieldInputLayout = EntryLayoutPrivate::convert(configGroup.readEntry(QString("fieldInputLayout%1").arg(field), "SingleLine"));
            if (sfl.bibtexLabel.isEmpty() || sfl.uiLabel.isEmpty())
                continue;

            etl->singleFieldLayouts.append(sfl);
        }
        append(etl);
    }

    if (isEmpty()) qWarning() << "List of entry layouts is empty";
}

void EntryLayout::save()
{
    int tabCount = 0;
    for (QVector<QSharedPointer<EntryTabLayout> >::ConstIterator it = constBegin(); it != constEnd(); ++it) {
        ++tabCount;
        QString groupName = QString("EntryLayoutTab%1").arg(tabCount);
        KConfigGroup configGroup(d->layoutConfig, groupName);

        configGroup.writeEntry("uiCaption", (*it)->uiCaption);
        configGroup.writeEntry("iconName", (*it)->iconName);
        configGroup.writeEntry("columns", (*it)->columns);

        int fieldCount = 0;
        foreach(const SingleFieldLayout &sfl, (*it)->singleFieldLayouts) {
            ++fieldCount;
            configGroup.writeEntry(QString("bibtexLabel%1").arg(fieldCount), sfl.bibtexLabel);
            configGroup.writeEntry(QString("uiLabel%1").arg(fieldCount), sfl.uiLabel);
            configGroup.writeEntry(QString("fieldInputLayout%1").arg(fieldCount), EntryLayoutPrivate::convert(sfl.fieldInputLayout));
        }
        configGroup.writeEntry("count", fieldCount);
    }

    QString groupName = QLatin1String("EntryLayoutTab");
    KConfigGroup configGroup(d->layoutConfig, groupName);
    configGroup.writeEntry("count", tabCount);

    d->layoutConfig->sync();
}

void EntryLayout::resetToDefaults()
{
    QString groupName = QLatin1String("EntryLayoutTab");
    KConfigGroup configGroup(d->layoutConfig, groupName);
    configGroup.deleteGroup();

    for (int tab = 1; tab < entryLayoutMaxTabCount; ++tab) {
        QString groupName = QString("EntryLayoutTab%1").arg(tab);
        KConfigGroup configGroup(d->layoutConfig, groupName);
        configGroup.deleteGroup();
    }

    load();
}
