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

#include "bibtexfields.h"

#include <QExplicitlySharedDataPointer>
#include <QStandardPaths>

#ifdef HAVE_KF5
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#else // HAVE_KF5
#define i18n(text) QObject::tr(text)
#endif // HAVE KF5

#include "preferences.h"
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

    BibTeXFieldsPrivate(const QString &style, BibTeXFields *parent)
            : p(parent) {
#ifdef HAVE_KF5
        const QString stylefile = style + QStringLiteral(".kbstyle");
        layoutConfig = KSharedConfig::openConfig(stylefile, KConfig::FullConfig, QStandardPaths::AppDataLocation);
        if (layoutConfig->groupList().isEmpty())
            qCWarning(LOG_KBIBTEX_CONFIG) << "The configuration file for layout of type" << style << "could not be located or is empty";
#else // HAVE_KF5
        Q_UNUSED(style)
#endif // HAVE_KF5
    }

#ifdef HAVE_KF5
    void load() {
        int logicalIndex = 0;
        for (BibTeXFields::Iterator it = p->begin(); it != p->end(); ++logicalIndex, ++it) {
            auto &fd = *it;
            const QString groupName = QStringLiteral("Column") + fd.upperCamelCase + fd.upperCamelCaseAlt;
            KConfigGroup configGroup(layoutConfig, groupName);

            fd.visible.clear();
            fd.width.clear();
            fd.visualIndex.clear();
            if (configGroup.exists()) {
                const QStringList keyList = configGroup.keyList();
                for (const QString &key : keyList) {
                    if (key.startsWith(QStringLiteral("Visible_"))) {
                        const QString treeViewName = key.mid(8);
                        fd.visible.insert(treeViewName, configGroup.readEntry(key, fd.defaultVisible));
                    } else if (key.startsWith(QStringLiteral("Width_"))) {
                        const QString treeViewName = key.mid(14);
                        /// Fall-back value of '0' means in BasicFileView to enable automatic column balancing if all fields have width 0
                        fd.width.insert(treeViewName, configGroup.readEntry(key, 0));
                    } else if (key.startsWith(QStringLiteral("VisualIndex_"))) {
                        const QString treeViewName = key.mid(12);
                        /// By default, visual index == logical index
                        fd.visualIndex.insert(treeViewName, configGroup.readEntry(key, logicalIndex));
                    }
                }
            }
        }
    }

    void save() {
        int logicalIndex = 0;
        for (const auto &fd : const_cast<const BibTeXFields &>(*p)) {
            const QString groupName = QStringLiteral("Column") + fd.upperCamelCase + fd.upperCamelCaseAlt;
            KConfigGroup configGroup(layoutConfig, groupName);

            const QList<QString> keys = fd.visible.keys(); ///< Assuming the keys are the same as for 'width' and 'visualIndex'
            for (const QString &treeViewName : keys) {
                const QString keyVisible = QStringLiteral("Visible_") + treeViewName;
                configGroup.writeEntry(keyVisible, fd.visible.value(treeViewName, fd.defaultVisible));
                const QString keyWidth = QStringLiteral("Width_") + treeViewName;
                /// Fall-back value of '0' means in BasicFileView to enable automatic column balancing if all fields have width 0
                configGroup.writeEntry(keyWidth, fd.width.value(treeViewName, 0));
                const QString keyVisualIndex = QStringLiteral("VisualIndex_") + treeViewName;
                /// By default, visual index == logical index
                configGroup.writeEntry(keyVisualIndex, fd.visualIndex.value(treeViewName, logicalIndex));
            }
            ++logicalIndex;
        }

        layoutConfig->sync();
    }

    void resetToDefaults(const QString &treeViewName) {
        for (BibTeXFields::Iterator it = p->begin(); it != p->end(); ++it) {
            const QString groupName = QStringLiteral("Column") + it->upperCamelCase + it->upperCamelCaseAlt;
            KConfigGroup configGroup(layoutConfig, groupName);
            configGroup.deleteEntry("Visible_" + treeViewName);
            configGroup.deleteEntry("Width_" + treeViewName);
            configGroup.deleteEntry("VisualIndex_" + treeViewName);
        }
        layoutConfig->sync();
        load();
    }
#endif // HAVE_KF5
};


BibTeXFields::BibTeXFields(const QString &style, const QVector<FieldDescription> &other)
        : QVector<FieldDescription>(other), d(new BibTeXFieldsPrivate(style, this))
{
#ifdef HAVE_KF5
    d->load();
#endif // HAVE_KF5
}

BibTeXFields::~BibTeXFields()
{
    delete d;
}

/// This function cannot return a 'const' BibTeXFields object
/// similarly like BibTeXEntries::instance as the FieldDescription's
/// QMap visible will be modified.
BibTeXFields &BibTeXFields::instance()
{
    static const QVector<FieldDescription> fieldDescriptionsBibTeX {
        FieldDescription {QStringLiteral("^type"), QString(), {}, i18n("Element Type"), KBibTeX::TypeFlag::Source, KBibTeX::TypeFlag::Source, {}, {}, 5, {}, true, true},
        FieldDescription {QStringLiteral("^id"), QString(), {}, i18n("Identifier"), KBibTeX::TypeFlag::Source, KBibTeX::TypeFlag::Source, {}, {}, 6, {}, true, true},
        FieldDescription {QStringLiteral("Title"), QString(), {}, i18n("Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("Title"), QStringLiteral("BookTitle"), {}, i18n("Title or Book Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, true, false},
        FieldDescription {QStringLiteral("Author"), QStringLiteral("Editor"), {}, i18n("Author or Editor"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 7, {}, true, false},
        FieldDescription {QStringLiteral("Author"), QString(), {}, i18n("Author"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Editor"), QString(), {}, i18n("Editor"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Month"), QString(), {}, i18n("Month"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, false},
        FieldDescription {QStringLiteral("Year"), QString(), {}, i18n("Year"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, true, false},
        FieldDescription {QStringLiteral("Journal"), QString(), {}, i18n("Journal"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 4, {}, false, false},
        FieldDescription {QStringLiteral("Volume"), QString(), {}, i18n("Volume"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Number"), QString(), {}, i18n("Number"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("ISSN"), QString(), {}, i18n("ISSN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("ISBN"), QString(), {}, i18n("ISBN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("ISBN"), QStringLiteral("ISSN"), {}, i18n("ISBN or ISSN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("HowPublished"), QString(), {}, i18n("How Published"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("Note"), QString(), {}, i18n("Note"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("Abstract"), QString(), {}, i18n("Abstract"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 7, {}, false, true},
        FieldDescription {QStringLiteral("Pages"), QString(), {}, i18n("Pages"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, true, false},
        FieldDescription {QStringLiteral("Publisher"), QString(), {}, i18n("Publisher"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("Institution"), QString(), {}, i18n("Institution"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("BookTitle"), QString(), {}, i18n("Book Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("Series"), QString(), {}, i18n("Series"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 12, {}, false, false},
        FieldDescription {QStringLiteral("Edition"), QString(), {}, i18n("Edition"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Chapter"), QString(), {}, i18n("Chapter"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Organization"), QString(), {}, i18n("Organization"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("School"), QString(), {}, i18n("School"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Keywords"), QString(), {}, i18n("Keywords"), KBibTeX::TypeFlag::Keyword, KBibTeX::TypeFlag::Keyword | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, true},
        FieldDescription {QStringLiteral("CrossRef"), QString(), {}, i18n("Cross Reference"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 3, {}, false, true},
        FieldDescription {QStringLiteral("DOI"), QString(), {}, i18n("DOI"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 1, {}, false, true},
        FieldDescription {QStringLiteral("URL"), QString(), {}, i18n("URL"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 3, {}, false, true},
        FieldDescription {QStringLiteral("Location"), QString(), {}, i18n("Location"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, false},
        FieldDescription {QStringLiteral("Address"), QString(), {}, i18n("Address"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, false},
        FieldDescription {QStringLiteral("Type"), QString(), {}, i18n("Type"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Key"), QString(), {}, i18n("Key"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("X-Color"), QString(), {}, i18n("Color"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("X-Stars"), QString(), {}, i18n("Stars"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim | KBibTeX::TypeFlag::Source, {}, {}, 4, {}, false, true},
    };
    static const QVector<FieldDescription> fieldDescriptionsBibLaTeX {
        FieldDescription {QStringLiteral("^type"), QString(), {}, i18n("Element Type"), KBibTeX::TypeFlag::Source, KBibTeX::TypeFlag::Source, {}, {}, 5, {}, true, true},
        FieldDescription {QStringLiteral("^id"), QString(), {}, i18n("Identifier"), KBibTeX::TypeFlag::Source, KBibTeX::TypeFlag::Source, {}, {}, 6, {}, true, true},
        FieldDescription {QStringLiteral("Title"), QString(), {}, i18n("Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("Title"), QStringLiteral("BookTitle"), {}, i18n("Title or Book Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, true, false},
        FieldDescription {QStringLiteral("SubTitle"), QString(), {}, i18n("Subtitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("TitleAddon"), QString(), {}, i18n("Title Addon"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("ShortTitle"), QString(), {}, i18n("Shortitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 4, {}, false, false},
        FieldDescription {QStringLiteral("OrigTitle"), QString(), {}, i18n("Original Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("ReprintTitle"), QString(), {}, i18n("Reprint Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("MainTitle"), QString(), {}, i18n("Main Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("MainSubTitle"), QString(), {}, i18n("Main Subtitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("MainTitleAddon"), QString(), {}, i18n("Maintitle Addon"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("Author"), QStringLiteral("Editor"), {}, i18n("Author or Editor"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 7, {}, true, false},
        FieldDescription {QStringLiteral("Author"), QString(), {}, i18n("Author"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("ShortAuthor"), QString(), {}, i18n("Short Author"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("NameAddon"), QString(), {}, i18n("Name Addon"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("AuthorType"), QString(), {}, i18n("Author Type"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("BookAuthor"), QString(), {}, i18n("Book Author"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Editor"), QString(), {}, i18n("Editor"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("ShortEditor"), QString(), {}, i18n("Short Editor"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("EditorType"), QString(), {}, i18n("Editor Type"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("EditorA"), QString(), {}, i18n("Editor A"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("EditorAType"), QString(), {}, i18n("Editor A Type"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("EditorB"), QString(), {}, i18n("Editor B"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("EditorBType"), QString(), {}, i18n("Editor B Type"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("EditorC"), QString(), {}, i18n("Editor C"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("EditorCType"), QString(), {}, i18n("Editor C Type"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Translator"), QString(), {}, i18n("Translator"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Afterword"), QString(), {}, i18n("Afterword Author"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Introduction"), QString(), {}, i18n("Introduction Author"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Foreword"), QString(), {}, i18n("Foreword Author"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Annotator"), QString(), {}, i18n("Annotator"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Commentator"), QString(), {}, i18n("Commentator"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Holder"), QString(), {}, i18n("Patent Holder"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("Month"), QString(), {}, i18n("Month"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, false},
        FieldDescription {QStringLiteral("Year"), QString(), {}, i18n("Year"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Date"), QString(), {}, i18n("Date"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Year"), QStringLiteral("Date"), {}, i18n("Date or Year"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, true, false},
        FieldDescription {QStringLiteral("EventDate"), QString(), {}, i18n("Event Date"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("OrigDate"), QString(), {}, i18n("Original Date"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("JournalTitle"), QString(), {QStringLiteral("Journal")}, i18n("Journal Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 4, {}, false, false},
        FieldDescription {QStringLiteral("JournalSubTitle"), QString(), {}, i18n("Journal Subtitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 4, {}, false, false},
        FieldDescription {QStringLiteral("ShortJournal"), QString(), {}, i18n("Journal Shortitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 4, {}, false, false},
        FieldDescription {QStringLiteral("Volume"), QString(), {}, i18n("Volume"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Volumes"), QString(), {}, i18n("Number of Volumes"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Number"), QString(), {}, i18n("Number"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Version"), QString(), {}, i18n("Version"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Part"), QString(), {}, i18n("Part"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Issue"), QString(), {}, i18n("Issue"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("IASN"), QString(), {}, i18n("IASN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("ISMN"), QString(), {}, i18n("ISMN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("ISRN"), QString(), {}, i18n("ISRN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("ISSN"), QString(), {}, i18n("ISSN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("ISBN"), QString(), {}, i18n("ISBN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("ISBN"), QStringLiteral("ISSN"), {}, i18n("ISBN or ISSN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("ISWC"), QString(), {}, i18n("ISWC"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("HowPublished"), QString(), {}, i18n("How Published"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("Note"), QString(), {}, i18n("Note"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("Addendum"), QString(), {}, i18n("Addendum"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("Annotation"), QString(), {QStringLiteral("Annote")}, i18n("Annotation"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("Abstract"), QString(), {}, i18n("Abstract"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 7, {}, false, true},
        FieldDescription {QStringLiteral("Pages"), QString(), {}, i18n("Pages"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, true, false},
        FieldDescription {QStringLiteral("PageTotal"), QString(), {}, i18n("Total Pages"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Pagination"), QString(), {}, i18n("Pagination"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("BookPagination"), QString(), {}, i18n("Book Pagination"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Publisher"), QString(), {}, i18n("Publisher"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("OrigPublisher"), QString(), {}, i18n("Original Publisher"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Institution"), QString(), {QStringLiteral("School")}, i18n("Institution"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 5, {}, false, false},
        FieldDescription {QStringLiteral("BookTitle"), QString(), {}, i18n("Book Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("BookSubTitle"), QString(), {}, i18n("Book Subtitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("IssueTitle"), QString(), {}, i18n("Issue Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("IssueSubTitle"), QString(), {}, i18n("Issue Subtitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("BookTitleAddon"), QString(), {}, i18n("Booktitle Addon"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 14, {}, false, false},
        FieldDescription {QStringLiteral("Series"), QString(), {}, i18n("Series"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 12, {}, false, false},
        FieldDescription {QStringLiteral("ShortSeries"), QString(), {}, i18n("Series Shortitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 4, {}, false, false},
        FieldDescription {QStringLiteral("Edition"), QString(), {}, i18n("Edition"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Chapter"), QString(), {}, i18n("Chapter"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("Organization"), QString(), {}, i18n("Organization"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("EventTitle"), QString(), {}, i18n("Event Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Venue"), QString(), {}, i18n("Venue"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("IndexTitle"), QString(), {}, i18n("Index Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("Keywords"), QString(), {}, i18n("Keywords"), KBibTeX::TypeFlag::Keyword, KBibTeX::TypeFlag::Keyword | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, true},
        FieldDescription {QStringLiteral("CrossRef"), QString(), {}, i18n("Cross Reference"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 3, {}, false, true},
        FieldDescription {QStringLiteral("XRef"), QString(), {}, i18n("XRef"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 3, {}, false, true},
        FieldDescription {QStringLiteral("DOI"), QString(), {}, i18n("DOI"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("EPrint"), QString(), {}, i18n("E-Print"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 1, {}, false, false},
        FieldDescription {QStringLiteral("EPrintClass"), QString(), {QStringLiteral("PrimaryClass")}, i18n("E-Print Class"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("EPrintType"), QString(), {QStringLiteral("ArchivePrefix")}, i18n("E-Print Type"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("URL"), QString(), {}, i18n("URL"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 3, {}, false, false},
        FieldDescription {QStringLiteral("URLDate"), QString(), {}, i18n("URL Date"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, false},
        FieldDescription {QStringLiteral("File"), QString(), {QStringLiteral("PDF")}, i18n("Local File URL"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim, {}, {}, 3, {}, false, true},
        FieldDescription {QStringLiteral("Location"), QString(), {QStringLiteral("Address")}, i18n("Location"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, false},
        FieldDescription {QStringLiteral("OrigLocation"), QString(), {}, i18n("Original Location"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Type"), QString(), {}, i18n("Type"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("EID"), QString(), {}, i18n("EID"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Label"), QString(), {}, i18n("Label"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("ShortHand"), QString(), {}, i18n("Shorthand"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("ShortHandIntro"), QString(), {}, i18n("Shorthand Intro"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("PubState"), QString(), {}, i18n("Publication State"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Language"), QString(), {}, i18n("Language"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("OrigLanguage"), QString(), {}, i18n("Original Language"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Library"), QString(), {}, i18n("Library"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("X-Color"), QString(), {}, i18n("Color"), KBibTeX::TypeFlag::Verbatim, KBibTeX::TypeFlag::Verbatim | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("Gender"), QString(), {}, i18n("Gender"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("Hyphenation"), QString(), {}, i18n("Hyphenation"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("IndexSortTitle"), QString(), {}, i18n("Index Sorttitle"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("Options"), QString(), {}, i18n("Entry Options"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("Presort"), QString(), {}, i18n("Presort"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("SortKey"), QString(), {QStringLiteral("Key")}, i18n("Sort Key"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("SortName"), QString(), {}, i18n("Sort Names"), KBibTeX::TypeFlag::Person, KBibTeX::TypeFlag::Person | KBibTeX::TypeFlag::Reference, {}, {}, 7, {}, false, false},
        FieldDescription {QStringLiteral("SortShortHand"), QString(), {}, i18n("Sort Shorthand"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("SortTitle"), QString(), {}, i18n("Sort Title"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("SortYear"), QString(), {}, i18n("Sort Year"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, true},
        FieldDescription {QStringLiteral("ISAN"), QString(), {}, i18n("ISAN"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 2, {}, false, false},
        FieldDescription {QStringLiteral("Location"), QString(), {}, i18n("Location"), KBibTeX::TypeFlag::PlainText, KBibTeX::TypeFlag::PlainText | KBibTeX::TypeFlag::Reference | KBibTeX::TypeFlag::Source, {}, {}, 3, {}, false, false},
    };

    static BibTeXFields singletonBibTeX(QStringLiteral("bibtex"), fieldDescriptionsBibTeX), singletonBibLaTeX(QStringLiteral("biblatex"), fieldDescriptionsBibLaTeX);

    return Preferences::instance().bibliographySystem() == Preferences::BibliographySystem::BibLaTeX ? singletonBibLaTeX : singletonBibTeX;
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
    case KBibTeX::Casing::LowerCase: return iName;
    case KBibTeX::Casing::UpperCase: return name.toUpper();
    case KBibTeX::Casing::InitialCapital:
        iName[0] = iName[0].toUpper();
        return iName;
    case KBibTeX::Casing::LowerCamelCase: {
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
    case KBibTeX::Casing::UpperCamelCase: {
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
    return FieldDescription {QString(), QString(), {}, QString(), KBibTeX::TypeFlag::Source, KBibTeX::TypeFlag::Source, {}, {}, 0, {}, false, false};
}

KBibTeX::TypeFlag BibTeXFields::typeFlagFromString(const QString &typeFlagString)
{
    KBibTeX::TypeFlag result = KBibTeX::TypeFlag::Invalid;

    if (typeFlagString == QStringLiteral("Text"))
        result = KBibTeX::TypeFlag::PlainText;
    else if (typeFlagString == QStringLiteral("Source"))
        result = KBibTeX::TypeFlag::Source;
    else if (typeFlagString == QStringLiteral("Person"))
        result = KBibTeX::TypeFlag::Person;
    else if (typeFlagString == QStringLiteral("Keyword"))
        result = KBibTeX::TypeFlag::Keyword;
    else if (typeFlagString == QStringLiteral("Reference"))
        result = KBibTeX::TypeFlag::Reference;
    else if (typeFlagString == QStringLiteral("Verbatim"))
        result = KBibTeX::TypeFlag::Verbatim;
    else
        qCWarning(LOG_KBIBTEX_CONFIG) << "Could not interpret string" << typeFlagString << "into a KBibTeX::TypeFlag value";

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
    if (typeFlags & KBibTeX::TypeFlag::PlainText) resultList << QStringLiteral("Text");
    if (typeFlags & KBibTeX::TypeFlag::Source) resultList << QStringLiteral("Source");
    if (typeFlags & KBibTeX::TypeFlag::Person) resultList << QStringLiteral("Person");
    if (typeFlags & KBibTeX::TypeFlag::Keyword) resultList << QStringLiteral("Keyword");
    if (typeFlags & KBibTeX::TypeFlag::Reference) resultList << QStringLiteral("Reference");
    if (typeFlags & KBibTeX::TypeFlag::Verbatim) resultList << QStringLiteral("Verbatim");
    return resultList.join(QChar(';'));
}

QString BibTeXFields::typeFlagToString(KBibTeX::TypeFlag typeFlag)
{
    if (typeFlag == KBibTeX::TypeFlag::PlainText) return QStringLiteral("Text");
    if (typeFlag == KBibTeX::TypeFlag::Source) return QStringLiteral("Source");
    if (typeFlag == KBibTeX::TypeFlag::Person) return QStringLiteral("Person");
    if (typeFlag == KBibTeX::TypeFlag::Keyword) return QStringLiteral("Keyword");
    if (typeFlag == KBibTeX::TypeFlag::Reference) return QStringLiteral("Reference");
    if (typeFlag == KBibTeX::TypeFlag::Verbatim) return QStringLiteral("Verbatim");
    return QString();
}
