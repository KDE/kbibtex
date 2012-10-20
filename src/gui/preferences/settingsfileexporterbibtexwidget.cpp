/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#include <QAbstractItemModel>

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
    KComboBox *comboBoxPersonNameFormatting;
    const Person dummyPerson;

    KSharedConfigPtr config;
    const QString configGroupName;

public:

    SettingsFileExporterBibTeXWidgetPrivate(SettingsFileExporterBibTeXWidget *parent)
            : p(parent), dummyPerson(Person(i18n("John"), i18n("Doe"), i18n("Jr."))), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("FileExporterBibTeX")) {
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
        QString personNameFormatting = configGroup.readEntry(Person::keyPersonNameFormatting, "");
        p->selectValue(comboBoxPersonNameFormatting, personNameFormatting, Qt::UserRole);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(FileExporterBibTeX::keyEncoding, comboBoxEncodings->currentText());
        QString stringDelimiter = comboBoxStringDelimiters->currentText();
        configGroup.writeEntry(FileExporterBibTeX::keyStringDelimiter, QString(stringDelimiter[0]) + stringDelimiter[stringDelimiter.length() - 1]);
        FileExporterBibTeX::QuoteComment quoteComment = (FileExporterBibTeX::QuoteComment)comboBoxQuoteComment->currentIndex();
        configGroup.writeEntry(FileExporterBibTeX::keyQuoteComment, (int)quoteComment);
        KBibTeX::Casing keywordCasing = (KBibTeX::Casing)comboBoxKeywordCasing->currentIndex();
        configGroup.writeEntry(FileExporterBibTeX::keyKeywordCasing, (int)keywordCasing);
        configGroup.writeEntry(FileExporterBibTeX::keyProtectCasing, checkBoxProtectCasing->isChecked());
        configGroup.writeEntry(Person::keyPersonNameFormatting, comboBoxPersonNameFormatting->itemData(comboBoxPersonNameFormatting->currentIndex()));

        config->sync();
    }

    void resetToDefaults() {
        p->selectValue(comboBoxEncodings, FileExporterBibTeX::defaultEncoding);
        p->selectValue(comboBoxStringDelimiters, createDelimiterString(FileExporterBibTeX::defaultStringDelimiter[0], FileExporterBibTeX::defaultStringDelimiter[1]));
        comboBoxQuoteComment->setCurrentIndex((int)FileExporterBibTeX::defaultQuoteComment);
        comboBoxKeywordCasing->setCurrentIndex((int)FileExporterBibTeX::defaultKeywordCasing);
        checkBoxProtectCasing->setChecked(FileExporterBibTeX::defaultProtectCasing);
        comboBoxPersonNameFormatting->setCurrentIndex(0);
    }

    void setupGUI() {
        QFormLayout *layout = new QFormLayout(p);

        comboBoxEncodings = new KComboBox(false, p);
        comboBoxEncodings->setObjectName("comboBoxEncodings");
        layout->addRow(i18n("Encoding:"), comboBoxEncodings);
        comboBoxEncodings->addItem(QLatin1String("LaTeX"));
        comboBoxEncodings->insertSeparator(1);
        comboBoxEncodings->addItems(IConvLaTeX::encodings());
        connect(comboBoxEncodings, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));

        comboBoxStringDelimiters = new KComboBox(false, p);
        comboBoxStringDelimiters->setObjectName("comboBoxStringDelimiters");
        layout->addRow(i18n("String Delimiters:"), comboBoxStringDelimiters);
        comboBoxStringDelimiters->addItem(createDelimiterString('"', '"'));
        comboBoxStringDelimiters->addItem(createDelimiterString('{', '}'));
        comboBoxStringDelimiters->addItem(createDelimiterString('(', ')'));
        connect(comboBoxStringDelimiters, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));

        comboBoxQuoteComment = new KComboBox(false, p);
        layout->addRow(i18n("Comment Quoting:"), comboBoxQuoteComment);
        comboBoxQuoteComment->addItem(i18nc("Comment Quoting", "None"));
        comboBoxQuoteComment->addItem(i18nc("Comment Quoting", "@comment{%1}", QChar(8230)));
        comboBoxQuoteComment->addItem(i18nc("Comment Quoting", "%{%1}", QChar(8230)));
        connect(comboBoxQuoteComment, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));

        comboBoxKeywordCasing = new KComboBox(false, p);
        layout->addRow(i18n("Keyword Casing:"), comboBoxKeywordCasing);
        comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "lowercase"));
        comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "Initial capital"));
        comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "UpperCamelCase"));
        comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "lowerCamelCase"));
        comboBoxKeywordCasing->addItem(i18nc("Keyword Casing", "UPPERCASE"));
        connect(comboBoxKeywordCasing, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));

        checkBoxProtectCasing = new QCheckBox(i18n("Protect Titles"));
        layout->addRow(i18n("Protect Casing?"), checkBoxProtectCasing);
        connect(checkBoxProtectCasing, SIGNAL(toggled(bool)), p, SIGNAL(changed()));

        comboBoxPersonNameFormatting = new KComboBox(false, p);
        comboBoxPersonNameFormatting->setObjectName("comboBoxPersonNameFormatting");
        layout->addRow(i18n("Person Names Formatting:"), comboBoxPersonNameFormatting);
        ItalicTextItemModel *itim = new ItalicTextItemModel();
        itim->addItem(i18n("Use global settings"), QString(""));
        itim->addItem(Person::transcribePersonName(&dummyPerson, QLatin1String("<%f ><%l>< %s>")), QLatin1String("<%f ><%l>< %s>"));
        itim->addItem(Person::transcribePersonName(&dummyPerson, QLatin1String("<%l><, %s><, %f>")), QLatin1String("<%l><, %s><, %f>"));
        comboBoxPersonNameFormatting->setModel(itim);
        connect(comboBoxPersonNameFormatting, SIGNAL(currentIndexChanged(int)), p, SIGNAL(changed()));
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
        if (file->hasProperty(File::NameFormatting))
            p->selectValue(comboBoxPersonNameFormatting, file->property(File::NameFormatting).toString(), Qt::UserRole);
    }

    void saveProperties(File *file) {
        file->setProperty(File::Encoding, comboBoxEncodings->currentText());
        QString stringDelimiter = comboBoxStringDelimiters->currentText();
        file->setProperty(File::StringDelimiter, QString(stringDelimiter[0]) + stringDelimiter[stringDelimiter.length() - 1]);
        FileExporterBibTeX::QuoteComment quoteComment = (FileExporterBibTeX::QuoteComment)comboBoxQuoteComment->currentIndex();
        file->setProperty(File::QuoteComment, (int)quoteComment);
        KBibTeX::Casing keywordCasing = (KBibTeX::Casing)comboBoxKeywordCasing->currentIndex();
        file->setProperty(File::KeywordCasing, (int)keywordCasing);
        file->setProperty(File::ProtectCasing, checkBoxProtectCasing->isChecked());
        file->setProperty(File::NameFormatting, comboBoxPersonNameFormatting->itemData(comboBoxPersonNameFormatting->currentIndex()));
    }
};


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

SettingsFileExporterBibTeXWidget::~SettingsFileExporterBibTeXWidget()
{
    delete d;
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
