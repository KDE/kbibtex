/***************************************************************************
 *   Copyright (C) 2004-2012 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
#include <QLabel>
#include <QFont>

#include <KLocale>

#include "statistics.h"
#include "element.h"
#include "file.h"
#include "entry.h"
#include "macro.h"
#include "comment.h"

class Statistics::StatisticsPrivate
{
private:
    Statistics *p;
    QLabel *labelNumberOfElements, *labelNumberOfEntries, *labelNumberOfJournalArticles, *labelNumberOfConferencePublications, *labelNumberOfBooks, *labelNumberOfComments, *labelNumberOfMacros;

public:
    const File *file;

    StatisticsPrivate(Statistics *parent)
            : p(parent) {
        QFormLayout *layout = new QFormLayout(parent);

        labelNumberOfElements = new QLabel(parent);
        setBold(labelNumberOfElements);
        layout->addRow(i18n("Number of Elements:"), labelNumberOfElements);

        labelNumberOfEntries = new QLabel(parent);
        setBold(labelNumberOfEntries);
        layout->addRow(i18n("Number of Entries:"), labelNumberOfEntries);

        labelNumberOfJournalArticles = new QLabel(parent);
        layout->addRow(i18n("Journal Articles:"), labelNumberOfJournalArticles);

        labelNumberOfConferencePublications = new QLabel(parent);
        layout->addRow(i18n("Conference Publications:"), labelNumberOfConferencePublications);

        labelNumberOfBooks = new QLabel(parent);
        layout->addRow(i18n("Books:"), labelNumberOfBooks);

        labelNumberOfComments = new QLabel(parent);
        setBold(labelNumberOfComments);
        layout->addRow(i18n("Number of Comments:"), labelNumberOfComments);

        labelNumberOfMacros = new QLabel(parent);
        setBold(labelNumberOfMacros);
        layout->addRow(i18n("Number of Macros:"), labelNumberOfMacros);
    }

    void setBold(QLabel *label) {
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }

    void update() {
        if (file != NULL) {
            int numEntries, numJournalArticles, numConferencePublications, numBooks, numComments, numMacros;
            countElementTypes(file, numEntries, numJournalArticles, numConferencePublications, numBooks, numComments, numMacros);

            labelNumberOfElements->setText(QString::number(file->count()));
            labelNumberOfEntries->setText(QString::number(numEntries));
            labelNumberOfJournalArticles->setText(QString::number(numJournalArticles));
            labelNumberOfConferencePublications->setText(QString::number(numConferencePublications));
            labelNumberOfBooks->setText(QString::number(numBooks));
            labelNumberOfComments->setText(QString::number(numComments));
            labelNumberOfMacros->setText(QString::number(numMacros));
        } else {
            labelNumberOfElements->setText(QChar(0x2013));
            labelNumberOfEntries->setText(QChar(0x2013));
            labelNumberOfJournalArticles->setText(QChar(0x2013));
            labelNumberOfConferencePublications->setText(QChar(0x2013));
            labelNumberOfBooks->setText(QChar(0x2013));
            labelNumberOfComments->setText(QChar(0x2013));
            labelNumberOfMacros->setText(QChar(0x2013));
        }
    }

    void countElementTypes(const File *file, int &numEntries, int &numJournalArticles, int &numConferencePublications, int &numBooks, int &numComments, int &numMacros) {
        numEntries = numJournalArticles = numConferencePublications = numBooks = numComments = numMacros = 0;
        for (File::ConstIterator it = file->constBegin(); it != file->constEnd(); ++it) {
            const QSharedPointer<const Element> &element = *it;
            const QSharedPointer<const Entry> entry = element.dynamicCast<const Entry>();
            if (!entry.isNull()) {
                if (entry->type().toLower() == Entry::etArticle)
                    ++numJournalArticles;
                else if (entry->type().toLower() == Entry::etInProceedings)
                    ++numConferencePublications;
                else if (entry->type().toLower() == Entry::etBook)
                    ++numBooks;
                ++numEntries;
            } else if (!element.dynamicCast<const Macro>().isNull())
                ++numMacros;
            else if (!element.dynamicCast<const Comment>().isNull())
                ++numComments;
        }
    }
};

Statistics::Statistics(QWidget *parent)
        : QWidget(parent), d(new StatisticsPrivate(this))
{
    // nothing
}

Statistics::~Statistics()
{
    delete d;
}

void Statistics::setFile(const File *file)
{
    d->file = file;
    update();
}

void Statistics::update()
{
    d->update();
}
