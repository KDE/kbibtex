/*****************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   *
 *                                                                           *
 *                                                                           *
 *   This program is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation; either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.    *
 *****************************************************************************/

#include "filesettingswidget.h"

#include <QCheckBox>
#include <QFormLayout>

#include <KComboBox>
#include <KLocale>

#include "preferences.h"
#include "italictextitemmodel.h"
#include "iconvlatex.h"
#include "file.h"
#include "guihelper.h"

#define createDelimiterString(a, b) (QString("%1%2%3").arg(a).arg(QChar(8230)).arg(b))

FileSettingsWidget::FileSettingsWidget(QWidget *parent)
        : QWidget(parent), dummyPerson(Person(i18n("John"), i18n("Doe"), i18n("Jr."))), m_file(NULL)
{
    setupGUI();
}

void FileSettingsWidget::loadProperties()
{
    if (m_file != NULL)
        loadProperties(m_file);
}

void FileSettingsWidget::loadProperties(File *file)
{
    m_file = file;
    if (file->hasProperty(File::Encoding)) {
        m_comboBoxEncodings->blockSignals(true);
        QString encoding = file->property(File::Encoding).toString();
        int row = GUIHelper::selectValue(m_comboBoxEncodings->model(), encoding);
        m_comboBoxEncodings->setCurrentIndex(row);
        m_comboBoxEncodings->blockSignals(false);
    }
    if (file->hasProperty(File::StringDelimiter)) {
        m_comboBoxStringDelimiters->blockSignals(true);
        QString stringDelimiter = file->property(File::StringDelimiter).toString();
        int row = GUIHelper::selectValue(m_comboBoxStringDelimiters->model(), createDelimiterString(stringDelimiter[0], stringDelimiter[1]));
        m_comboBoxStringDelimiters->setCurrentIndex(row);
        m_comboBoxStringDelimiters->blockSignals(false);
    }
    if (file->hasProperty(File::QuoteComment)) {
        m_comboBoxQuoteComment->blockSignals(true);
        Preferences::QuoteComment quoteComment = (Preferences::QuoteComment)file->property(File::QuoteComment).toInt();
        m_comboBoxQuoteComment->setCurrentIndex((int)quoteComment);
        m_comboBoxQuoteComment->blockSignals(false);
    }
    if (file->hasProperty(File::KeywordCasing)) {
        m_comboBoxKeywordCasing->blockSignals(true);
        KBibTeX::Casing keywordCasing = (KBibTeX::Casing)file->property(File::KeywordCasing).toInt();
        m_comboBoxKeywordCasing->setCurrentIndex((int)keywordCasing);
        m_comboBoxKeywordCasing->blockSignals(false);
    }
    if (file->hasProperty(File::ProtectCasing)) {
        m_checkBoxProtectCasing->blockSignals(true);
        m_checkBoxProtectCasing->setChecked(file->property(File::ProtectCasing).toBool());
        m_checkBoxProtectCasing->blockSignals(false);
    }
    if (file->hasProperty(File::NameFormatting)) {
        m_comboBoxPersonNameFormatting->blockSignals(true);
        int row = GUIHelper::selectValue(m_comboBoxPersonNameFormatting->model(), file->property(File::NameFormatting).toString(), ItalicTextItemModel::IdentifierRole);
        m_comboBoxPersonNameFormatting->setCurrentIndex(row);
        m_comboBoxPersonNameFormatting->blockSignals(false);
    }
    if (file->hasProperty(File::ListSeparator)) {
        m_comboBoxListSeparator->blockSignals(true);
        m_comboBoxListSeparator->setCurrentIndex(m_comboBoxListSeparator->findData(file->property(File::ListSeparator)));
        m_comboBoxListSeparator->blockSignals(false);
    }
}

void FileSettingsWidget::saveProperties()
{
    if (m_file != NULL)
        saveProperties(m_file);
}

void FileSettingsWidget::saveProperties(File *file)
{
    m_file = file;
    file->setProperty(File::Encoding, m_comboBoxEncodings->currentText());
    QString stringDelimiter = m_comboBoxStringDelimiters->currentText();
    file->setProperty(File::StringDelimiter, QString(stringDelimiter[0]) + stringDelimiter[stringDelimiter.length() - 1]);
    Preferences::QuoteComment quoteComment = (Preferences::QuoteComment)m_comboBoxQuoteComment->currentIndex();
    file->setProperty(File::QuoteComment, (int)quoteComment);
    KBibTeX::Casing keywordCasing = (KBibTeX::Casing)m_comboBoxKeywordCasing->currentIndex();
    file->setProperty(File::KeywordCasing, (int)keywordCasing);
    file->setProperty(File::ProtectCasing, m_checkBoxProtectCasing->isChecked());
    file->setProperty(File::NameFormatting, m_comboBoxPersonNameFormatting->itemData(m_comboBoxPersonNameFormatting->currentIndex(), ItalicTextItemModel::IdentifierRole));
    file->setProperty(File::ListSeparator, m_comboBoxListSeparator->itemData(m_comboBoxListSeparator->currentIndex()).toString());
}

void FileSettingsWidget::resetToDefaults()
{
    if (m_file != NULL) {
        m_file->setPropertiesToDefault();
        loadProperties(m_file);
    }
}

void FileSettingsWidget::setupGUI()
{
    QFormLayout *layout = new QFormLayout(this);

    m_comboBoxEncodings = new KComboBox(false, this);
    m_comboBoxEncodings->setObjectName("comboBoxEncodings");
    layout->addRow(i18n("Encoding:"), m_comboBoxEncodings);
    m_comboBoxEncodings->addItem(QLatin1String("LaTeX"));
    m_comboBoxEncodings->insertSeparator(1);
    m_comboBoxEncodings->addItems(IConvLaTeX::encodings());
    connect(m_comboBoxEncodings, SIGNAL(currentIndexChanged(int)), this, SIGNAL(widgetsChanged()));

    m_comboBoxStringDelimiters = new KComboBox(false, this);
    m_comboBoxStringDelimiters->setObjectName("comboBoxStringDelimiters");
    layout->addRow(i18n("String Delimiters:"), m_comboBoxStringDelimiters);
    m_comboBoxStringDelimiters->addItem(createDelimiterString('"', '"'));
    m_comboBoxStringDelimiters->addItem(createDelimiterString('{', '}'));
    m_comboBoxStringDelimiters->addItem(createDelimiterString('(', ')'));
    connect(m_comboBoxStringDelimiters, SIGNAL(currentIndexChanged(int)), this, SIGNAL(widgetsChanged()));

    m_comboBoxQuoteComment = new KComboBox(false, this);
    layout->addRow(i18n("Comment Quoting:"), m_comboBoxQuoteComment);
    m_comboBoxQuoteComment->addItem(i18nc("Comment Quoting", "None"));
    m_comboBoxQuoteComment->addItem(i18nc("Comment Quoting", "@comment{%1}", QChar(8230)));
    m_comboBoxQuoteComment->addItem(i18nc("Comment Quoting", "%{%1}", QChar(8230)));
    connect(m_comboBoxQuoteComment, SIGNAL(currentIndexChanged(int)), this, SIGNAL(widgetsChanged()));

    m_comboBoxKeywordCasing = new KComboBox(false, this);
    layout->addRow(i18n("Keyword Casing:"), m_comboBoxKeywordCasing);
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "lowercase"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "Initial capital"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "UpperCamelCase"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "lowerCamelCase"));
    m_comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "UPPERCASE"));
    connect(m_comboBoxKeywordCasing, SIGNAL(currentIndexChanged(int)), this, SIGNAL(widgetsChanged()));

    m_checkBoxProtectCasing = new QCheckBox(i18n("Protect Titles"), this);
    layout->addRow(i18n("Protect Casing?"), m_checkBoxProtectCasing);
    connect(m_checkBoxProtectCasing, SIGNAL(toggled(bool)), this, SIGNAL(widgetsChanged()));

    m_comboBoxPersonNameFormatting = new KComboBox(false, this);
    m_comboBoxPersonNameFormatting->setObjectName("comboBoxPersonNameFormatting");
    layout->addRow(i18n("Person Names Formatting:"), m_comboBoxPersonNameFormatting);
    connect(m_comboBoxPersonNameFormatting, SIGNAL(currentIndexChanged(int)), this, SIGNAL(widgetsChanged()));

    ItalicTextItemModel *itim = new ItalicTextItemModel();
    itim->addItem(i18n("Use global settings"), QString(""));
    itim->addItem(Person::transcribePersonName(&dummyPerson, Preferences::personNameFormatFirstLast), Preferences::personNameFormatFirstLast);
    itim->addItem(Person::transcribePersonName(&dummyPerson, Preferences::personNameFormatLastFirst), Preferences::personNameFormatLastFirst);
    m_comboBoxPersonNameFormatting->setModel(itim);

    m_comboBoxListSeparator = new KComboBox(false, this);
    layout->addRow(i18n("List Separator"), m_comboBoxListSeparator);
    connect(m_comboBoxListSeparator, SIGNAL(currentIndexChanged(int)), this, SIGNAL(widgetsChanged()));
    m_comboBoxListSeparator->addItem(QLatin1String(";"), QVariant::fromValue<QString>(QLatin1String("; ")));
    m_comboBoxListSeparator->addItem(QLatin1String(","), QVariant::fromValue<QString>(QLatin1String(", ")));
}
