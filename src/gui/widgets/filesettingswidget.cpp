/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "filesettingswidget.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#include <QRegExpValidator>
#else // #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QRegularExpressionValidator>
#endif // #if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))

#include <KLocalizedString>

#include <Preferences>
#include <File>
#include "italictextitemmodel.h"
#include "guihelper.h"

#define createDelimiterString(a, b) (QString(QStringLiteral("%1%2%3")).arg(a).arg(QChar(8230)).arg(b))

FileSettingsWidget::FileSettingsWidget(QWidget *parent)
        : QWidget(parent), dummyPerson(Person(i18n("John"), i18n("Doe"), i18n("Jr."))), m_file(nullptr)
{
    setupGUI();
}

void FileSettingsWidget::resetToLoadedProperties()
{
    loadProperties(m_file);
}

void FileSettingsWidget::loadProperties(File *file)
{
    m_file = file;
    if (m_file == nullptr) return; /// Nothing to do

    if (file->hasProperty(File::Encoding)) {
        QSignalBlocker comboBoxEncodingsSignalBlocker(m_comboBoxEncodings);
        const QString encoding = file->property(File::Encoding).toString();
        const int row = GUIHelper::selectValue(m_comboBoxEncodings->model(), encoding);
        m_comboBoxEncodings->setCurrentIndex(row);
    }
    if (file->hasProperty(File::StringDelimiter)) {
        QSignalBlocker comboBoxStringDelimitersSignalBlocker(m_comboBoxStringDelimiters);
        const QString stringDelimiter = file->property(File::StringDelimiter).toString();
        const int row = GUIHelper::selectValue(m_comboBoxStringDelimiters->model(), createDelimiterString(stringDelimiter[0], stringDelimiter[1]));
        m_comboBoxStringDelimiters->setCurrentIndex(row);
    }
    if (file->hasProperty(File::CommentContext)) {
        QSignalBlocker comboBoxCommentContextSignalBlocker(m_comboBoxCommentContext);
        const Preferences::CommentContext commentContext = static_cast<Preferences::CommentContext>(file->property(File::CommentContext).toInt());
        const int row = qMax(0, GUIHelper::selectValue(m_comboBoxCommentContext->model(), static_cast<int>(commentContext), Qt::UserRole));
        m_comboBoxCommentContext->setCurrentIndex(row);
    }
    if (file->hasProperty(File::CommentPrefix)) {
        QSignalBlocker lineEditCommentPrefixSignalBlocker(m_lineEditCommentPrefix);
        const QString commentPrefix = file->property(File::CommentPrefix).toString();
        m_lineEditCommentPrefix->setText(commentPrefix);
    }
    if (file->hasProperty(File::KeywordCasing)) {
        QSignalBlocker comboBoxKeywordCasingSignalBlocker(m_comboBoxKeywordCasing);
        const KBibTeX::Casing keywordCasing = static_cast<KBibTeX::Casing>(file->property(File::KeywordCasing).toInt());
        m_comboBoxKeywordCasing->setCurrentIndex(static_cast<int>(keywordCasing));
    }
    if (file->hasProperty(File::ProtectCasing)) {
        QSignalBlocker checkBoxProtectCasingSignalBlocker(m_checkBoxProtectCasing);
        m_checkBoxProtectCasing->setCheckState(static_cast<Qt::CheckState>(file->property(File::ProtectCasing).toInt()));
    }
    if (file->hasProperty(File::NameFormatting)) {
        QSignalBlocker comboBoxPersonNameFormattingSignalBlocker(m_comboBoxPersonNameFormatting);
        const int row = GUIHelper::selectValue(m_comboBoxPersonNameFormatting->model(), file->property(File::NameFormatting).toString(), ItalicTextItemModel::IdentifierRole);
        m_comboBoxPersonNameFormatting->setCurrentIndex(row);
    }
    if (file->hasProperty(File::ListSeparator)) {
        QSignalBlocker comboBoxListSeparatorSignalBlocker(m_comboBoxListSeparator);
        m_comboBoxListSeparator->setCurrentIndex(m_comboBoxListSeparator->findData(file->property(File::ListSeparator)));
    }
    if (file->hasProperty(File::SortedByIdentifier)) {
        QSignalBlocker checkBoxSortedByIdentifierSignalBlocker(m_checkBoxSortedByIdentifier);
        m_checkBoxSortedByIdentifier->setChecked(file->property(File::SortedByIdentifier).toBool());
    }
}

void FileSettingsWidget::saveProperties(File *file)
{
    m_file = file;
    if (m_file == nullptr) return;

    file->setProperty(File::Encoding, m_comboBoxEncodings->currentText());
    const QString stringDelimiter = m_comboBoxStringDelimiters->currentText();
    file->setProperty(File::StringDelimiter, QString(QStringLiteral("%1%2")).arg(stringDelimiter[0], stringDelimiter[stringDelimiter.length() - 1]));
    bool ok = false;
    const Preferences::CommentContext commentContext = static_cast<Preferences::CommentContext>(m_comboBoxCommentContext->currentData().toInt(&ok));
    if (ok) {
        file->setProperty(File::CommentContext, static_cast<int>(commentContext));
        if (commentContext == Preferences::CommentContext::Prefix && m_lineEditCommentPrefix->hasAcceptableInput())
            file->setProperty(File::CommentPrefix, m_lineEditCommentPrefix->text());
    }
    const KBibTeX::Casing keywordCasing = static_cast<KBibTeX::Casing>(m_comboBoxKeywordCasing->currentIndex());
    file->setProperty(File::KeywordCasing, static_cast<int>(keywordCasing));
    file->setProperty(File::ProtectCasing, static_cast<int>(m_checkBoxProtectCasing->checkState()));
    file->setProperty(File::NameFormatting, m_comboBoxPersonNameFormatting->itemData(m_comboBoxPersonNameFormatting->currentIndex(), ItalicTextItemModel::IdentifierRole));
    file->setProperty(File::ListSeparator, m_comboBoxListSeparator->itemData(m_comboBoxListSeparator->currentIndex()).toString());
    file->setProperty(File::SortedByIdentifier, m_checkBoxSortedByIdentifier->isChecked());
}

void FileSettingsWidget::resetToDefaults()
{
    if (m_file != nullptr) {
        m_file->setPropertiesToDefault();
        loadProperties(m_file);
    }
}

void FileSettingsWidget::setupGUI()
{
    QFormLayout *layout = new QFormLayout(this);

    m_comboBoxEncodings = new QComboBox(this);
    m_comboBoxEncodings->setObjectName(QStringLiteral("comboBoxEncodings"));
    layout->addRow(i18n("Encoding:"), m_comboBoxEncodings);
    m_comboBoxEncodings->addItems(Preferences::availableBibTeXEncodings);
    connect(m_comboBoxEncodings, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSettingsWidget::widgetsChanged);

    m_comboBoxStringDelimiters = new QComboBox(this);
    m_comboBoxStringDelimiters->setObjectName(QStringLiteral("comboBoxStringDelimiters"));
    layout->addRow(i18n("String Delimiters:"), m_comboBoxStringDelimiters);
    m_comboBoxStringDelimiters->addItem(createDelimiterString('"', '"'));
    m_comboBoxStringDelimiters->addItem(createDelimiterString('{', '}'));
    m_comboBoxStringDelimiters->addItem(createDelimiterString('(', ')'));
    connect(m_comboBoxStringDelimiters, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSettingsWidget::widgetsChanged);

    m_comboBoxCommentContext = new QComboBox(this);
    layout->addRow(i18n("Comment Context:"), m_comboBoxCommentContext);
    for (QVector<QPair<Preferences::CommentContext, QString>>::ConstIterator it = Preferences::availableBibTeXCommentContexts.constBegin(); it != Preferences::availableBibTeXCommentContexts.constEnd(); ++it)
        m_comboBoxCommentContext->addItem(it->second, static_cast<int>(it->first));
    connect(m_comboBoxCommentContext, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSettingsWidget::widgetsChanged);

    m_lineEditCommentPrefix = new QLineEdit(this);
    layout->addRow(i18n("Comment Prefix:"), m_lineEditCommentPrefix);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    m_lineEditCommentPrefix->setValidator(new QRegExpValidator(QRegExp(QStringLiteral("^%.*"))));
#else // #if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    m_lineEditCommentPrefix->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^%.*"))));
#endif // #if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    connect(m_lineEditCommentPrefix, &QLineEdit::textEdited, this, &FileSettingsWidget::widgetsChanged);
    connect(m_comboBoxCommentContext, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this]() {
        bool ok = false;
        const Preferences::CommentContext currentCommentContext = static_cast<Preferences::CommentContext>(m_comboBoxCommentContext->currentData().toInt(&ok));
        m_lineEditCommentPrefix->setEnabled(ok && currentCommentContext == Preferences::CommentContext::Prefix);
    });

    m_comboBoxKeywordCasing = new QComboBox(this);
    layout->addRow(i18n("Keyword Casing:"), m_comboBoxKeywordCasing);
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "lowercase"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "Initial capital"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "UpperCamelCase"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "lowerCamelCase"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "UPPERCASE"));
    connect(m_comboBoxKeywordCasing, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSettingsWidget::widgetsChanged);

    m_checkBoxProtectCasing = new QCheckBox(i18n("Protect Titles"), this);
    m_checkBoxProtectCasing->setTristate(true);
    layout->addRow(i18n("Protect Casing?"), m_checkBoxProtectCasing);
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
    connect(m_checkBoxProtectCasing, &QCheckBox::stateChanged, this, &FileSettingsWidget::widgetsChanged);
#else // (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
    connect(m_checkBoxProtectCasing, &QCheckBox::checkStateChanged, this, &FileSettingsWidget::widgetsChanged);
#endif // (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))

    m_comboBoxPersonNameFormatting = new QComboBox(this);
    m_comboBoxPersonNameFormatting->setObjectName(QStringLiteral("comboBoxPersonNameFormatting"));
    layout->addRow(i18n("Person Names Formatting:"), m_comboBoxPersonNameFormatting);
    connect(m_comboBoxPersonNameFormatting, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSettingsWidget::widgetsChanged);

    ItalicTextItemModel *itim = new ItalicTextItemModel(m_comboBoxPersonNameFormatting);
    itim->addItem(i18n("Use global settings"), QString(QString()));
    itim->addItem(Person::transcribePersonName(&dummyPerson, Preferences::personNameFormatFirstLast), Preferences::personNameFormatFirstLast);
    itim->addItem(Person::transcribePersonName(&dummyPerson, Preferences::personNameFormatLastFirst), Preferences::personNameFormatLastFirst);
    m_comboBoxPersonNameFormatting->setModel(itim);

    m_comboBoxListSeparator = new QComboBox(this);
    layout->addRow(i18n("List Separator"), m_comboBoxListSeparator);
    connect(m_comboBoxListSeparator, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &FileSettingsWidget::widgetsChanged);
    m_comboBoxListSeparator->addItem(QStringLiteral(";"), QVariant::fromValue<QString>(QStringLiteral("; ")));
    m_comboBoxListSeparator->addItem(QStringLiteral(","), QVariant::fromValue<QString>(QStringLiteral(", ")));

    m_checkBoxSortedByIdentifier = new QCheckBox(i18n("Sort by identifier"), this);
    layout->addRow(i18n("Entry Sorting"), m_checkBoxSortedByIdentifier);
#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
    connect(m_checkBoxSortedByIdentifier, &QCheckBox::stateChanged, this, &FileSettingsWidget::widgetsChanged);
#else // (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
    connect(m_checkBoxSortedByIdentifier, &QCheckBox::checkStateChanged, this, &FileSettingsWidget::widgetsChanged);
#endif // (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
}
