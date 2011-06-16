/***************************************************************************
*   Copyright (C) 2004-2011 by Thomas Fischer                             *
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

#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocale>
#include <KComboBox>

#include <kbibtexnamespace.h>
#include <fileexporterbibtex.h>
#include <iconvlatex.h>
#include <file.h>
#include "settingsfileexporterbibtexwidget.h"

#define createDelimiterString(a, b) (QString("%1%2%3").arg(a).arg(QChar(8230)).arg(b))

class SettingsFileExporterBibTeXWidget::SettingsFileExporterBibTeXWidgetPrivate
{
private:
    SettingsFileExporterBibTeXWidget *p;

    KComboBox *comboBoxEncodings;
    KComboBox *comboBoxStringDelimiters;
    KComboBox *comboBoxQuoteComment;
    KComboBox *comboBoxKeywordCasing;
    QCheckBox *checkBoxProtectCasing;

    KSharedConfigPtr config;
    static const QString configGroupName;

public:

    SettingsFileExporterBibTeXWidgetPrivate(SettingsFileExporterBibTeXWidget *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))) {
        // nothing
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        QString encoding = configGroup.readEntry(FileExporterBibTeX::keyEncoding, FileExporterBibTeX::defaultEncoding);
        p->selectValue(comboBoxEncodings, encoding);
        QString stringDelimiter = configGroup.readEntry(FileExporterBibTeX::keyStringDelimiter, FileExporterBibTeX::defaultStringDelimiter);
        p->selectValue(comboBoxStringDelimiters, createDelimiterString(stringDelimiter[0], stringDelimiter[1]));
        FileExporterBibTeX::QuoteComment quoteComment = (FileExporterBibTeX::QuoteComment)configGroup.readEntry(FileExporterBibTeX::keyQuoteComment, (int)FileExporterBibTeX::defaultQuoteComment);
        comboBoxQuoteComment->setCurrentIndex((int)quoteComment);
        KBibTeX::Casing keywordCasing = (KBibTeX::Casing)configGroup.readEntry(FileExporterBibTeX::keyKeywordCasing, (int)FileExporterBibTeX::defaultKeywordCasing);
        comboBoxKeywordCasing->setCurrentIndex((int)keywordCasing);
        checkBoxProtectCasing->setChecked(configGroup.readEntry(FileExporterBibTeX::keyProtectCasing, FileExporterBibTeX::defaultProtectCasing));
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(FileExporterBibTeX::keyEncoding, comboBoxEncodings->currentText());
        QString stringDelimiter = comboBoxStringDelimiters->currentText();
        configGroup.writeEntry(FileExporterBibTeX::keyStringDelimiter, QString(stringDelimiter[0]) + stringDelimiter[stringDelimiter.length()-1]);
        FileExporterBibTeX::QuoteComment quoteComment = (FileExporterBibTeX::QuoteComment)comboBoxQuoteComment->currentIndex();
        configGroup.writeEntry(FileExporterBibTeX::keyQuoteComment, (int)quoteComment);
        KBibTeX::Casing keywordCasing = (KBibTeX::Casing)comboBoxKeywordCasing->currentIndex();
        configGroup.writeEntry(FileExporterBibTeX::keyKeywordCasing, (int)keywordCasing);
        configGroup.writeEntry(FileExporterBibTeX::keyProtectCasing, checkBoxProtectCasing->isChecked());

        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxEncodings, FileExporterBibTeX::defaultEncoding);
        p->selectValue(comboBoxStringDelimiters, createDelimiterString(FileExporterBibTeX::defaultStringDelimiter[0], FileExporterBibTeX::defaultStringDelimiter[1]));
        comboBoxQuoteComment->setCurrentIndex((int)FileExporterBibTeX::defaultQuoteComment);
        comboBoxKeywordCasing->setCurrentIndex((int)FileExporterBibTeX::defaultKeywordCasing);
        checkBoxProtectCasing->setChecked(FileExporterBibTeX::defaultProtectCasing);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxEncodings = new KComboBox(false, p);
        layout->addRow(i18n("Encoding:"), comboBoxEncodings);
        comboBoxEncodings->addItem(QLatin1String("LaTeX"));
        comboBoxEncodings->insertSeparator(1);
        comboBoxEncodings->addItems(IConvLaTeX::encodings());

        comboBoxStringDelimiters = new KComboBox(false, p);
        layout->addRow(i18n("String Delimiters:"), comboBoxStringDelimiters);
        comboBoxStringDelimiters->addItem(createDelimiterString('"', '"'));
        comboBoxStringDelimiters->addItem(createDelimiterString('{', '}'));
        comboBoxStringDelimiters->addItem(createDelimiterString('(', ')'));

        comboBoxQuoteComment = new KComboBox(false, p);
        layout->addRow(i18n("Comment Quoting:"), comboBoxQuoteComment);
        comboBoxQuoteComment->addItem(i18n("None"));
        comboBoxQuoteComment->addItem(i18n("@comment{%1}", QChar(8230)));
        comboBoxQuoteComment->addItem(i18n("%{%1}", QChar(8230)));

        comboBoxKeywordCasing = new KComboBox(false, p);
        layout->addRow(i18n("Keyword Casing:"), comboBoxKeywordCasing);
        comboBoxKeywordCasing->addItem(i18n("lowercase"));
        comboBoxKeywordCasing->addItem(i18n("Initial Capital"));
        comboBoxKeywordCasing->addItem(i18n("UpperCamelCase"));
        comboBoxKeywordCasing->addItem(i18n("lowerCamelCase"));
        comboBoxKeywordCasing->addItem(i18n("UPPERCASE"));

        checkBoxProtectCasing = new QCheckBox(i18n("Protect Titles"));
        layout->addRow(i18n("Protect Casing?"), checkBoxProtectCasing);
    }

    void loadProperties(File *file) {
        if (file->hasProperty(File::Encoding)) {
            QString encoding = file->property(File::Encoding).toString();
            p->selectValue(comboBoxEncodings, encoding);
        }
        if (file->hasProperty(File::StringDelimiter)) {
            QString stringDelimiter = file->property(File::StringDelimiter).toString();
            p->selectValue(comboBoxStringDelimiters, createDelimiterString(stringDelimiter[0], stringDelimiter[1]));
        }
        if (file->hasProperty(File::QuoteComment)) {
            FileExporterBibTeX::QuoteComment quoteComment = (FileExporterBibTeX::QuoteComment)file->property(File::QuoteComment).toInt();
            comboBoxQuoteComment->setCurrentIndex((int)quoteComment);
        }
        if (file->hasProperty(File::KeywordCasing)) {
            KBibTeX::Casing keywordCasing = (KBibTeX::Casing)file->property(File::KeywordCasing).toInt();
            comboBoxKeywordCasing->setCurrentIndex((int)keywordCasing);
        }
        if (file->hasProperty(File::ProtectCasing))
            checkBoxProtectCasing->setChecked(file->property(File::QuoteComment).toBool());
    }

    void saveProperties(File *file) {
        file->setProperty(File::Encoding, comboBoxEncodings->currentText());
        QString stringDelimiter = comboBoxStringDelimiters->currentText();
        file->setProperty(File::StringDelimiter, QString(stringDelimiter[0]) + stringDelimiter[stringDelimiter.length()-1]);
        FileExporterBibTeX::QuoteComment quoteComment = (FileExporterBibTeX::QuoteComment)comboBoxQuoteComment->currentIndex();
        file->setProperty(File::QuoteComment, (int)quoteComment);
        KBibTeX::Casing keywordCasing = (KBibTeX::Casing)comboBoxKeywordCasing->currentIndex();
        file->setProperty(File::KeywordCasing, (int)keywordCasing);
        file->setProperty(File::ProtectCasing, checkBoxProtectCasing->isChecked());
    }
};

const QString SettingsFileExporterBibTeXWidget::SettingsFileExporterBibTeXWidgetPrivate::configGroupName = QLatin1String("FileExporterBibTeX");


SettingsFileExporterBibTeXWidget::SettingsFileExporterBibTeXWidget(QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterBibTeXWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
}

SettingsFileExporterBibTeXWidget::SettingsFileExporterBibTeXWidget(File *file, QWidget *parent)
        : SettingsAbstractWidget(parent), d(new SettingsFileExporterBibTeXWidgetPrivate(this))
{
    d->setupGUI();
    d->loadState();
    d->loadProperties(file);
}

void SettingsFileExporterBibTeXWidget::loadState()
{
    d->loadState();
}

void SettingsFileExporterBibTeXWidget::saveState()
{
    d->saveState();
}

void SettingsFileExporterBibTeXWidget::saveProperties(File *file)
{
    d->saveProperties(file);
}

void SettingsFileExporterBibTeXWidget::resetToDefaults()
{
    d->resetToDefaults();
}
