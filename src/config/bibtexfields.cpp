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

#include "bibtexfields.h"

#include <QExplicitlySharedDataPointer>
#include <QStandardPaths>

#ifdef HAVE_KF5
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#endif // HAVE KF5

#include "logging_config.h"

bool operator==(const FieldDescription &a, const FieldDescription &b)
{
    return a.upperCamelCase == b.upperCamelCase;
}
uint qHash(const FieldDescription &a)
{
    return qHash(a.upperCamelCase);
}

class BibTeXFields::BibTeXFieldsPrivate
{
public:
    static const QVector<FieldDescription> fieldDescriptionsBibTeX;
    static const QVector<FieldDescription> fieldDescriptionsBibLaTeX;

    BibTeXFields *p;

#ifdef HAVE_KF5
    KSharedConfigPtr layoutConfig;
#endif // HAVE_KF5

    static BibTeXFields *singleton;

    BibTeXFieldsPrivate(BibTeXFields *parent)
            : p(parent) {
#ifdef HAVE_KF5
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, QStringLiteral("User Interface"));
        const QString stylefile = configGroup.readEntry(QStringLiteral("CurrentStyle"), QString(QStringLiteral("bibtex"))).append(QStringLiteral(".kbstyle"));
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::AppDataLocation);
        if (layoutConfig->groupList().isEmpty())
            qCWarning(LOG_KBIBTEX_CONFIG) << "The configuration file for BibTeX fields could not be located or is empty:" << stylefile;
#endif // HAVE_KF5
    }

#ifdef HAVE_KF5
    void load() {
        for (BibTeXFields::Iterator it = p->begin(); it != p->end(); ++it) {
            auto &fd = *it;
            const QString groupName = QStringLiteral("Column") + fd.upperCamelCase + fd.upperCamelCaseAlt;
            KConfigGroup configGroup(layoutConfig, groupName);

            fd.visible.clear();
            if (configGroup.exists()) {
                const QStringList keyList = configGroup.keyList();
                for (const QString &key : keyList) {
                    if (!key.startsWith(QStringLiteral("Visible_"))) continue; ///< a key other than a 'visibility' key
                    const QString treeViewName = key.mid(8);
                    fd.visible.insert(treeViewName, configGroup.readEntry(key, fd.defaultVisible));
                }
            }
        }
    }

    void save() {
        for (const auto &fd : const_cast<const BibTeXFields &>(*p)) {
            const QString groupName = QStringLiteral("Column") + fd.upperCamelCase + fd.upperCamelCaseAlt;
            KConfigGroup configGroup(layoutConfig, groupName);

            const QList<QString> keys = fd.visible.keys();
            for (const QString &treeViewName : keys) {
                const QString key = QStringLiteral("Visible_") + treeViewName;
                configGroup.writeEntry(key, fd.visible.value(treeViewName, fd.defaultVisible));
            }
        }

        layoutConfig->sync();
    }

    void resetToDefaults(const QString &treeViewName) {
        for (BibTeXFields::Iterator it = p->begin(); it != p->end(); ++it) {
            const QString groupName = QStringLiteral("Column") + it->upperCamelCase + it->upperCamelCaseAlt;
            KConfigGroup configGroup(layoutConfig, groupName);
            configGroup.deleteEntry("Visible_" + treeViewName);
        }
        layoutConfig->sync();
        load();
    }
#endif // HAVE_KF5
};


BibTeXFields *BibTeXFields::BibTeXFieldsPrivate::singleton = nullptr;

BibTeXFields::BibTeXFields(const QVector<FieldDescription> &other)
        : QVector<FieldDescription>(other), d(new BibTeXFieldsPrivate(this))
{
#ifdef HAVE_KF5
    d->load();
#endif // HAVE_KF5
}

BibTeXFields::~BibTeXFields()
{
    delete d;
}

BibTeXFields *BibTeXFields::self()
{
    if (BibTeXFieldsPrivate::singleton == nullptr) {
        static const QVector<FieldDescription> fieldDescriptionsBibTeX {
            FieldDescription {QStringLiteral("^type"), QString(), i18n("Element Type"), KBibTeX::tfSource, KBibTeX::tfSource, 5, {}, true, true},
            FieldDescription {QStringLiteral("^id"), QString(), i18n("Identifier"), KBibTeX::tfSource, KBibTeX::tfSource, 6, {}, true, true},
            FieldDescription {QStringLiteral("Title"), QString(), i18n("Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("Title"), QStringLiteral("BookTitle"), i18n("Title or Book Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, true, false},
            FieldDescription {QStringLiteral("Author"), QStringLiteral("Editor"), i18n("Author or Editor"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 7, {}, true, false},
            FieldDescription {QStringLiteral("Author"), QString(), i18n("Author"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Editor"), QString(), i18n("Editor"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Month"), QString(), i18n("Month"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 3, {}, false, false},
            FieldDescription {QStringLiteral("Year"), QString(), i18n("Year"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, true, false},
            FieldDescription {QStringLiteral("Journal"), QString(), i18n("Journal"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 4, {}, false, false},
            FieldDescription {QStringLiteral("Volume"), QString(), i18n("Volume"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Number"), QString(), i18n("Number"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("ISSN"), QString(), i18n("ISSN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("ISBN"), QString(), i18n("ISBN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("ISBN"), QStringLiteral("ISSN"), i18n("ISBN or ISSN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("HowPublished"), QString(), i18n("How Published"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("Note"), QString(), i18n("Note"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("Abstract"), QString(), i18n("Abstract"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 7, {}, false, true},
            FieldDescription {QStringLiteral("Pages"), QString(), i18n("Pages"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, true, false},
            FieldDescription {QStringLiteral("Publisher"), QString(), i18n("Publisher"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("Institution"), QString(), i18n("Institution"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("BookTitle"), QString(), i18n("Book Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("Series"), QString(), i18n("Series"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 12, {}, false, false},
            FieldDescription {QStringLiteral("Edition"), QString(), i18n("Edition"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Chapter"), QString(), i18n("Chapter"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Organization"), QString(), i18n("Organization"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("School"), QString(), i18n("School"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Keywords"), QString(), i18n("Keywords"), KBibTeX::tfKeyword, KBibTeX::tfKeyword | KBibTeX::tfSource, 3, {}, false, true},
            FieldDescription {QStringLiteral("CrossRef"), QString(), i18n("Cross Reference"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 3, {}, false, true},
            FieldDescription {QStringLiteral("DOI"), QString(), i18n("DOI"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 1, {}, false, true},
            FieldDescription {QStringLiteral("URL"), QString(), i18n("URL"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 3, {}, false, true},
            FieldDescription {QStringLiteral("Address"), QString(), i18n("Address"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 3, {}, false, false},
            FieldDescription {QStringLiteral("Location"), QString(), i18n("Location"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 3, {}, false, false},
            FieldDescription {QStringLiteral("Type"), QString(), i18n("Type"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Key"), QString(), i18n("Key"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("X-Color"), QString(), i18n("Color"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("X-Stars"), QString(), i18n("Stars"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim | KBibTeX::tfSource, 4, {}, false, true},
        };
        static const QVector<FieldDescription> fieldDescriptionsBibLaTeX {
            FieldDescription {QStringLiteral("^type"), QString(), i18n("Element Type"), KBibTeX::tfSource, KBibTeX::tfSource, 5, {}, true, true},
            FieldDescription {QStringLiteral("^id"), QString(), i18n("Identifier"), KBibTeX::tfSource, KBibTeX::tfSource, 6, {}, true, true},
            FieldDescription {QStringLiteral("Title"), QString(), i18n("Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("Title"), QStringLiteral("BookTitle"), i18n("Title or Book Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, true, false},
            FieldDescription {QStringLiteral("SubTitle"), QString(), i18n("Subtitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("TitleAddon"), QString(), i18n("Title Addon"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("ShortTitle"), QString(), i18n("Shortitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 4, {}, false, false},
            FieldDescription {QStringLiteral("OrigTitle"), QString(), i18n("Original Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("ReprintTitle"), QString(), i18n("Reprint Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("MainTitle"), QString(), i18n("Main Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("MainSubTitle"), QString(), i18n("Main Subtitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("MainTitleAddon"), QString(), i18n("Maintitle Addon"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("Author"), QStringLiteral("Editor"), i18n("Author or Editor"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 7, {}, true, false},
            FieldDescription {QStringLiteral("Author"), QString(), i18n("Author"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("ShortAuthor"), QString(), i18n("Short Author"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("NameAddon"), QString(), i18n("Name Addon"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("AuthorType"), QString(), i18n("Author Type"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("BookAuthor"), QString(), i18n("Book Author"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Editor"), QString(), i18n("Editor"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("ShortEditor"), QString(), i18n("Short Editor"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("EditorType"), QString(), i18n("Editor Type"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("EditorA"), QString(), i18n("Editor A"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("EditorAType"), QString(), i18n("Editor A Type"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("EditorB"), QString(), i18n("Editor B"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("EditorBType"), QString(), i18n("Editor B Type"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("EditorC"), QString(), i18n("Editor C"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("EditorCType"), QString(), i18n("Editor C Type"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Translator"), QString(), i18n("Translator"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Afterword"), QString(), i18n("Afterword Author"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Introduction"), QString(), i18n("Introduction Author"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Foreword"), QString(), i18n("Foreword Author"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Annotator"), QString(), i18n("Annotator"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Commentator"), QString(), i18n("Commentator"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Holder"), QString(), i18n("Patent Holder"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("Month"), QString(), i18n("Month"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 3, {}, false, false},
            FieldDescription {QStringLiteral("Year"), QString(), i18n("Year"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Date"), QString(), i18n("Date"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("EventDate"), QString(), i18n("Event Date"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("OrigDate"), QString(), i18n("Original Date"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Year"), QStringLiteral("Date"), i18n("Date or Year"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, true, false},
            FieldDescription {QStringLiteral("JournalTitle"), QString(), i18n("Journal Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 4, {}, false, false},
            FieldDescription {QStringLiteral("JournalSubTitle"), QString(), i18n("Journal Subtitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 4, {}, false, false},
            FieldDescription {QStringLiteral("ShortJournal"), QString(), i18n("Journal Shortitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 4, {}, false, false},
            FieldDescription {QStringLiteral("Volume"), QString(), i18n("Volume"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Volumes"), QString(), i18n("Number of Volumes"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Number"), QString(), i18n("Number"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Version"), QString(), i18n("Version"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Part"), QString(), i18n("Part"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Issue"), QString(), i18n("Issue"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("IASN"), QString(), i18n("IASN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("ISMN"), QString(), i18n("ISMN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("ISRN"), QString(), i18n("ISRN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("ISSN"), QString(), i18n("ISSN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("ISBN"), QString(), i18n("ISBN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("ISBN"), QStringLiteral("ISSN"), i18n("ISBN or ISSN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("ISWC"), QString(), i18n("ISWC"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("HowPublished"), QString(), i18n("How Published"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("Note"), QString(), i18n("Note"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("Addendum"), QString(), i18n("Addendum"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("Annotation"), QString(), i18n("Annotation"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("Abstract"), QString(), i18n("Abstract"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 7, {}, false, true},
            FieldDescription {QStringLiteral("Pages"), QString(), i18n("Pages"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, true, false},
            FieldDescription {QStringLiteral("PageTotal"), QString(), i18n("Total Pages"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Pagination"), QString(), i18n("Pagination"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("BookPagination"), QString(), i18n("Book Pagination"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Publisher"), QString(), i18n("Publisher"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("OrigPublisher"), QString(), i18n("Original Publisher"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Institution"), QString(), i18n("Institution"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 5, {}, false, false},
            FieldDescription {QStringLiteral("BookTitle"), QString(), i18n("Book Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("BookSubTitle"), QString(), i18n("Book Subtitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("IssueTitle"), QString(), i18n("Issue Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("IssueSubTitle"), QString(), i18n("Issue Subtitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("BookTitleAddon"), QString(), i18n("Booktitle Addon"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 14, {}, false, false},
            FieldDescription {QStringLiteral("Series"), QString(), i18n("Series"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 12, {}, false, false},
            FieldDescription {QStringLiteral("ShortSeries"), QString(), i18n("Series Shortitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 4, {}, false, false},
            FieldDescription {QStringLiteral("Edition"), QString(), i18n("Edition"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Chapter"), QString(), i18n("Chapter"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 1, {}, false, false},
            FieldDescription {QStringLiteral("Organization"), QString(), i18n("Organization"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Institution"), QStringLiteral("School"), i18n("Institution"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("EventTitle"), QString(), i18n("Event Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Venue"), QString(), i18n("Venue"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("IndexTitle"), QString(), i18n("Index Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("Keywords"), QString(), i18n("Keywords"), KBibTeX::tfKeyword, KBibTeX::tfKeyword | KBibTeX::tfSource, 3, {}, false, true},
            FieldDescription {QStringLiteral("CrossRef"), QString(), i18n("Cross Reference"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 3, {}, false, true},
            FieldDescription {QStringLiteral("XRef"), QString(), i18n("XRef"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 3, {}, false, true},
            FieldDescription {QStringLiteral("DOI"), QString(), i18n("DOI"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 1, {}, false, false},
            FieldDescription {QStringLiteral("EPrint"), QString(), i18n("E-Print"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 1, {}, false, false},
            FieldDescription {QStringLiteral("EPrintClass"), QString(), i18n("E-Print Class"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("EPrintType"), QString(), i18n("E-Print Type"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("URL"), QString(), i18n("URL"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 3, {}, false, false},
            FieldDescription {QStringLiteral("URLDate"), QString(), i18n("URL Date"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 3, {}, false, false},
            FieldDescription {QStringLiteral("File"), QStringLiteral("PDF"), i18n("Local File URL"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim, 3, {}, false, true},
            FieldDescription {QStringLiteral("Location"), QStringLiteral("Address"), i18n("Location"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 3, {}, false, false},
            FieldDescription {QStringLiteral("OrigLocation"), QString(), i18n("Original Location"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Type"), QString(), i18n("Type"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("EID"), QString(), i18n("EID"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("SortKey"), QStringLiteral("Key"), i18n("Sort Key"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("Label"), QString(), i18n("Label"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("ShortHand"), QString(), i18n("Shorthand"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("ShortHandIntro"), QString(), i18n("Shorthand Intro"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("PubState"), QString(), i18n("Publication State"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Language"), QString(), i18n("Language"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("OrigLanguage"), QString(), i18n("Original Language"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Library"), QString(), i18n("Library"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("X-Color"), QString(), i18n("Color"), KBibTeX::tfVerbatim, KBibTeX::tfVerbatim | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("Gender"), QString(), i18n("Gender"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("Hyphenation"), QString(), i18n("Hyphenation"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("IndexSortTitle"), QString(), i18n("Index Sorttitle"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("Options"), QString(), i18n("Entry Options"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("Presort"), QString(), i18n("Presort"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("SortKey"), QString(), i18n("Sort Key"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("SortName"), QString(), i18n("Sort Names"), KBibTeX::tfPerson, KBibTeX::tfPerson | KBibTeX::tfReference, 7, {}, false, false},
            FieldDescription {QStringLiteral("SortShortHand"), QString(), i18n("Sort Shorthand"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("SortTitle"), QString(), i18n("Sort Title"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("SortYear"), QString(), i18n("Sort Year"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, true},
            FieldDescription {QStringLiteral("ISAN"), QString(), i18n("ISAN"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 2, {}, false, false},
            FieldDescription {QStringLiteral("Location"), QString(), i18n("Location"), KBibTeX::tfPlainText, KBibTeX::tfPlainText | KBibTeX::tfReference | KBibTeX::tfSource, 3, {}, false, false},
        };

        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, QStringLiteral("User Interface"));
        const QString stylefile = configGroup.readEntry("CurrentStyle", "bibtex");
        BibTeXFieldsPrivate::singleton = stylefile == QStringLiteral("biblatex") ? new BibTeXFields(fieldDescriptionsBibLaTeX) : new BibTeXFields(fieldDescriptionsBibTeX);
    }
    return BibTeXFieldsPrivate::singleton;
}

#ifdef HAVE_KF5
void BibTeXFields::save()
{
    d->save();
}

void BibTeXFields::resetToDefaults(const QString &treeViewName)
{
    d->resetToDefaults(treeViewName);
}
#endif // HAVE_KF5

QString BibTeXFields::format(const QString &name, KBibTeX::Casing casing) const
{
    QString iName = name.toLower();

    switch (casing) {
    case KBibTeX::cLowerCase: return iName;
    case KBibTeX::cUpperCase: return name.toUpper();
    case KBibTeX::cInitialCapital:
        iName[0] = iName[0].toUpper();
        return iName;
    case KBibTeX::cLowerCamelCase: {
        for (const auto &fd : const_cast<const BibTeXFields &>(*this)) {
            /// configuration file uses camel-case
            QString itName = fd.upperCamelCase.toLower();
            if (itName == iName && fd.upperCamelCaseAlt.isEmpty()) {
                iName = fd.upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toLower();
        return iName;
    }
    case KBibTeX::cUpperCamelCase: {
        for (const auto &fd : const_cast<const BibTeXFields &>(*this)) {
            /// configuration file uses camel-case
            QString itName = fd.upperCamelCase.toLower();
            if (itName == iName && fd.upperCamelCaseAlt.isEmpty()) {
                iName = fd.upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toUpper();
        return iName;
    }
    }
    return name;
}

const FieldDescription BibTeXFields::find(const QString &name) const
{
    const QString iName = name.toLower();
    for (const auto &fd : const_cast<const BibTeXFields &>(*this)) {
        if (fd.upperCamelCase.toLower() == iName && fd.upperCamelCaseAlt.isEmpty())
            return fd;
    }
    qCWarning(LOG_KBIBTEX_CONFIG) << "No field description for " << name << "(" << iName << ")";
    return FieldDescription {QString(), QString(), QString(), KBibTeX::tfSource, KBibTeX::tfSource, 0, {}, false, false};
}

KBibTeX::TypeFlag BibTeXFields::typeFlagFromString(const QString &typeFlagString)
{
    KBibTeX::TypeFlag result = (KBibTeX::TypeFlag)0;

    if (typeFlagString == QStringLiteral("Text"))
        result = KBibTeX::tfPlainText;
    else if (typeFlagString == QStringLiteral("Source"))
        result = KBibTeX::tfSource;
    else if (typeFlagString == QStringLiteral("Person"))
        result = KBibTeX::tfPerson;
    else if (typeFlagString == QStringLiteral("Keyword"))
        result = KBibTeX::tfKeyword;
    else if (typeFlagString == QStringLiteral("Reference"))
        result = KBibTeX::tfReference;
    else if (typeFlagString == QStringLiteral("Verbatim"))
        result = KBibTeX::tfVerbatim;

    return result;
}

KBibTeX::TypeFlags BibTeXFields::typeFlagsFromString(const QString &typeFlagsString)
{
    KBibTeX::TypeFlags result;

    const QStringList list = typeFlagsString.split(';');
    for (const QString &s : list)
        result |= typeFlagFromString(s);

    return result;
}

QString BibTeXFields::typeFlagsToString(KBibTeX::TypeFlags typeFlags)
{
    QStringList resultList;
    if (typeFlags & KBibTeX::tfPlainText) resultList << QStringLiteral("Text");
    if (typeFlags & KBibTeX::tfSource) resultList << QStringLiteral("Source");
    if (typeFlags & KBibTeX::tfPerson) resultList << QStringLiteral("Person");
    if (typeFlags & KBibTeX::tfKeyword) resultList << QStringLiteral("Keyword");
    if (typeFlags & KBibTeX::tfReference) resultList << QStringLiteral("Reference");
    if (typeFlags & KBibTeX::tfVerbatim) resultList << QStringLiteral("Verbatim");
    return resultList.join(QChar(';'));
}

QString BibTeXFields::typeFlagToString(KBibTeX::TypeFlag typeFlag)
{
    if (typeFlag == KBibTeX::tfPlainText) return QStringLiteral("Text");
    if (typeFlag == KBibTeX::tfSource) return QStringLiteral("Source");
    if (typeFlag == KBibTeX::tfPerson) return QStringLiteral("Person");
    if (typeFlag == KBibTeX::tfKeyword) return QStringLiteral("Keyword");
    if (typeFlag == KBibTeX::tfReference) return QStringLiteral("Reference");
    if (typeFlag == KBibTeX::tfVerbatim) return QStringLiteral("Verbatim");
    return QString();
}
