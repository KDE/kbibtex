/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2019 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include <Preferences>
#include "logging_gui.h"

static const int entryLayoutMaxTabCount = 256;
static const int entryLayoutMaxFieldPerTabCount = 256;

class EntryLayout::EntryLayoutPrivate
{
public:
    EntryLayout *p;

    EntryLayoutPrivate(EntryLayout *parent)
            : p(parent) {
        /// nothing
    }

    static QString convert(KBibTeX::FieldInputType fil) {
        switch (fil) {
        case KBibTeX::FieldInputType::SingleLine : return QStringLiteral("SingleLine");
        case KBibTeX::FieldInputType::MultiLine : return QStringLiteral("MultiLine");
        case KBibTeX::FieldInputType::List : return QStringLiteral("List");
        case KBibTeX::FieldInputType::Url : return QStringLiteral("URL");
        case KBibTeX::FieldInputType::Month : return QStringLiteral("Month");
        case KBibTeX::FieldInputType::Edition : return QStringLiteral("Edition");
        case KBibTeX::FieldInputType::Color : return QStringLiteral("Color");
        case KBibTeX::FieldInputType::PersonList : return QStringLiteral("PersonList");
        case KBibTeX::FieldInputType::KeywordList : return QStringLiteral("KeywordList");
        case KBibTeX::FieldInputType::CrossRef : return QStringLiteral("CrossRef");
        case KBibTeX::FieldInputType::StarRating : return QStringLiteral("StarRating");
        case KBibTeX::FieldInputType::UrlList : return QStringLiteral("UrlList");
        }
        return QString();
    }

    static KBibTeX::FieldInputType convert(const QString &text) {
        if (text == QStringLiteral("List"))
            return KBibTeX::FieldInputType::List;
        else if (text == QStringLiteral("MultiLine"))
            return KBibTeX::FieldInputType::MultiLine;
        else if (text == QStringLiteral("URL"))
            return KBibTeX::FieldInputType::Url;
        else if (text == QStringLiteral("UrlList"))
            return KBibTeX::FieldInputType::UrlList;
        else if (text == QStringLiteral("Month"))
            return KBibTeX::FieldInputType::Month;
        else if (text == QStringLiteral("Edition"))
            return KBibTeX::FieldInputType::Edition;
        else if (text == QStringLiteral("Color"))
            return KBibTeX::FieldInputType::Color;
        else if (text == QStringLiteral("PersonList"))
            return KBibTeX::FieldInputType::PersonList;
        else if (text == QStringLiteral("KeywordList"))
            return KBibTeX::FieldInputType::KeywordList;
        else if (text == QStringLiteral("CrossRef"))
            return KBibTeX::FieldInputType::CrossRef;
        else if (text == QStringLiteral("StarRating"))
            return KBibTeX::FieldInputType::StarRating;
        else
            return KBibTeX::FieldInputType::SingleLine;
    }

    void load(const QString &style)
    {
        p->clear();

        const QString stylefile = QStringLiteral("kbibtex/") + style + QStringLiteral(".kbstyle");
        KSharedConfigPtr layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::GenericDataLocation);
        static const QString groupName = QStringLiteral("EntryLayoutTab");
        const KConfigGroup configGroup(layoutConfig, groupName);
        const int tabCount = qMin(configGroup.readEntry("count", 0), entryLayoutMaxTabCount);

        for (int tab = 1; tab <= tabCount; ++tab) {
            const QString groupName = QString(QStringLiteral("EntryLayoutTab%1")).arg(tab);
            const KConfigGroup configGroup(layoutConfig, groupName);

            QSharedPointer<EntryTabLayout> etl = QSharedPointer<EntryTabLayout>(new EntryTabLayout);
            etl->identifier = configGroup.readEntry("identifier", QString(QStringLiteral("etl%1")).arg(tab));
            etl->uiCaption = i18n(configGroup.readEntry("uiCaption", QString()).toUtf8().constData());
            etl->iconName = configGroup.readEntry("iconName", "entry");
            etl->columns = configGroup.readEntry("columns", 1);
            if (etl->uiCaption.isEmpty())
                continue;

            const int fieldCount = qMin(configGroup.readEntry("count", 0), entryLayoutMaxFieldPerTabCount);
            for (int field = 1; field <= fieldCount; ++field) {
                SingleFieldLayout sfl;
                sfl.bibtexLabel = configGroup.readEntry(QString(QStringLiteral("bibtexLabel%1")).arg(field), QString());
                sfl.uiLabel = i18n(configGroup.readEntry(QString(QStringLiteral("uiLabel%1")).arg(field), QString()).toUtf8().constData());
                sfl.fieldInputLayout = EntryLayoutPrivate::convert(configGroup.readEntry(QString(QStringLiteral("fieldInputLayout%1")).arg(field), "SingleLine"));
                if (sfl.bibtexLabel.isEmpty() || sfl.uiLabel.isEmpty())
                    continue;

                etl->singleFieldLayouts.append(sfl);
            }

            const QString untranslatedInfoMessage = configGroup.readEntry("infoMessage", QString());
            if (!untranslatedInfoMessage.isEmpty()) {
                const QString infoMessagePipeSeparated = i18n(untranslatedInfoMessage.toUtf8().constData());
                if (!infoMessagePipeSeparated.isEmpty())
                    etl->infoMessages = infoMessagePipeSeparated.split(QLatin1Char('|'));
            }

            p->append(etl);
        }

        if (p->isEmpty()) qCWarning(LOG_KBIBTEX_GUI) << "List of entry layouts is empty";
    }
};

EntryLayout::EntryLayout(const QString &style)
        : QVector<QSharedPointer<EntryTabLayout> >(), d(new EntryLayoutPrivate(this))
{
    d->load(style);
}

EntryLayout::~EntryLayout()
{
    delete d;
}

const EntryLayout &EntryLayout::instance()
{
    static const EntryLayout singletonBibTeX(QStringLiteral("bibtex")), singletonBibLaTeX(QStringLiteral("biblatex"));
    return Preferences::instance().bibliographySystem() == Preferences::BibliographySystem::BibLaTeX ? singletonBibLaTeX : singletonBibTeX;
}
