/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "entrylayout.h"

#include <QStandardPaths>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include "logging_gui.h"

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
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, QString(QStringLiteral("User Interface")));
        const QString stylefile = configGroup.readEntry("CurrentStyle", "bibtex").append(".kbstyle").prepend("kbibtex/");
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::GenericDataLocation);
    }

    static QString convert(KBibTeX::FieldInputType fil) {
        switch (fil) {
        case KBibTeX::SingleLine : return QStringLiteral("SingleLine");
        case KBibTeX::MultiLine : return QStringLiteral("MultiLine");
        case KBibTeX::List : return QStringLiteral("List");
        case KBibTeX::URL : return QStringLiteral("URL");
        case KBibTeX::Month : return QStringLiteral("Month");
        case KBibTeX::Edition : return QStringLiteral("Edition");
        case KBibTeX::Color : return QStringLiteral("Color");
        case KBibTeX::PersonList : return QStringLiteral("PersonList");
        case KBibTeX::KeywordList : return QStringLiteral("KeywordList");
        case KBibTeX::CrossRef : return QStringLiteral("CrossRef");
        case KBibTeX::StarRating : return QStringLiteral("StarRating");
        case KBibTeX::UrlList : return QStringLiteral("UrlList");
        }
        return QString();
    }

    static KBibTeX::FieldInputType convert(const QString &text) {
        if (text == QStringLiteral("List"))
            return KBibTeX::List;
        else if (text == QStringLiteral("MultiLine"))
            return KBibTeX::MultiLine;
        else if (text == QStringLiteral("URL"))
            return KBibTeX::URL;
        else if (text == QStringLiteral("UrlList"))
            return KBibTeX::UrlList;
        else if (text == QStringLiteral("Month"))
            return KBibTeX::Month;
        else if (text == QStringLiteral("Edition"))
            return KBibTeX::Edition;
        else if (text == QStringLiteral("Color"))
            return KBibTeX::Color;
        else if (text == QStringLiteral("PersonList"))
            return KBibTeX::PersonList;
        else if (text == QStringLiteral("KeywordList"))
            return KBibTeX::KeywordList;
        else if (text == QStringLiteral("CrossRef"))
            return KBibTeX::CrossRef;
        else if (text == QStringLiteral("StarRating"))
            return KBibTeX::StarRating;
        else
            return KBibTeX::SingleLine;
    }
};

EntryLayout *EntryLayout::EntryLayoutPrivate::singleton = nullptr;

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
    if (EntryLayoutPrivate::singleton == nullptr)
        EntryLayoutPrivate::singleton  = new EntryLayout();
    return EntryLayoutPrivate::singleton;
}

void EntryLayout::load()
{
    clear();

    QString groupName = QStringLiteral("EntryLayoutTab");
    KConfigGroup configGroup(d->layoutConfig, groupName);
    int tabCount = qMin(configGroup.readEntry("count", 0), entryLayoutMaxTabCount);

    for (int tab = 1; tab <= tabCount; ++tab) {
        QString groupName = QString(QStringLiteral("EntryLayoutTab%1")).arg(tab);
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
            sfl.bibtexLabel = configGroup.readEntry(QString(QStringLiteral("bibtexLabel%1")).arg(field), QString());
            sfl.uiLabel = i18n(configGroup.readEntry(QString(QStringLiteral("uiLabel%1")).arg(field), QString()).toUtf8().constData());
            sfl.fieldInputLayout = EntryLayoutPrivate::convert(configGroup.readEntry(QString(QStringLiteral("fieldInputLayout%1")).arg(field), "SingleLine"));
            if (sfl.bibtexLabel.isEmpty() || sfl.uiLabel.isEmpty())
                continue;

            etl->singleFieldLayouts.append(sfl);
        }
        append(etl);
    }

    if (isEmpty()) qCWarning(LOG_KBIBTEX_GUI) << "List of entry layouts is empty";
}

void EntryLayout::save()
{
    int tabCount = 0;
    for (const auto &etl : const_cast<const EntryLayout &>(*this)) {
        ++tabCount;
        QString groupName = QString(QStringLiteral("EntryLayoutTab%1")).arg(tabCount);
        KConfigGroup configGroup(d->layoutConfig, groupName);

        configGroup.writeEntry(QStringLiteral("uiCaption"), etl->uiCaption);
        configGroup.writeEntry(QStringLiteral("iconName"), etl->iconName);
        configGroup.writeEntry(QStringLiteral("columns"), etl->columns);

        int fieldCount = 0;
        for (const SingleFieldLayout &sfl : const_cast<const QList<SingleFieldLayout> &>(etl->singleFieldLayouts)) {
            ++fieldCount;
            configGroup.writeEntry(QString(QStringLiteral("bibtexLabel%1")).arg(fieldCount), sfl.bibtexLabel);
            configGroup.writeEntry(QString(QStringLiteral("uiLabel%1")).arg(fieldCount), sfl.uiLabel);
            configGroup.writeEntry(QString(QStringLiteral("fieldInputLayout%1")).arg(fieldCount), EntryLayoutPrivate::convert(sfl.fieldInputLayout));
        }
        configGroup.writeEntry(QStringLiteral("count"), fieldCount);
    }

    QString groupName = QStringLiteral("EntryLayoutTab");
    KConfigGroup configGroup(d->layoutConfig, groupName);
    configGroup.writeEntry(QStringLiteral("count"), tabCount);

    d->layoutConfig->sync();
}

void EntryLayout::resetToDefaults()
{
    QString groupName = QStringLiteral("EntryLayoutTab");
    KConfigGroup configGroup(d->layoutConfig, groupName);
    configGroup.deleteGroup();

    for (int tab = 1; tab < entryLayoutMaxTabCount; ++tab) {
        QString groupName = QString(QStringLiteral("EntryLayoutTab%1")).arg(tab);
        KConfigGroup configGroup(d->layoutConfig, groupName);
        configGroup.deleteGroup();
    }

    load();
}
