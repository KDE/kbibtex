/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "statistics.h"

#include <QFormLayout>
#include <QLabel>
#include <QFont>
#include <QItemSelectionModel>

#include <KLocalizedString>

#include <File>
#include <file/FileView>
#include <Entry>
#include <Macro>
#include <Comment>
#include "openfileinfo.h"

class Statistics::StatisticsPrivate
{
private:
    // UNUSED Statistics *p;
    QLabel *labelNumberOfElements, *labelNumberOfEntries, *labelNumberOfJournalArticles, *labelNumberOfConferencePublications, *labelNumberOfBooks, *labelNumberOfOtherEntries, *labelNumberOfComments, *labelNumberOfMacros;

public:
    FileView *fileView;
    const File *file;
    const QItemSelectionModel *selectionModel;

    StatisticsPrivate(Statistics *parent)
        : /* UNUSED p(parent),*/ fileView(nullptr), file(nullptr), selectionModel(nullptr) {
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

        labelNumberOfOtherEntries = new QLabel(parent);
        layout->addRow(i18n("Other Entries:"), labelNumberOfOtherEntries);

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
        file = fileView != nullptr && fileView->fileModel() != nullptr ? fileView->fileModel()->bibliographyFile() : nullptr;
        selectionModel = fileView != nullptr ? fileView->selectionModel() : nullptr;

        if (file != nullptr) {
            int numElements, numEntries, numJournalArticles, numConferencePublications, numBooks, numComments, numMacros;
            countElementTypes(numElements, numEntries, numJournalArticles, numConferencePublications, numBooks, numComments, numMacros);

            labelNumberOfElements->setText(QString::number(numElements));
            labelNumberOfEntries->setText(QString::number(numEntries));
            labelNumberOfJournalArticles->setText(QString::number(numJournalArticles));
            labelNumberOfConferencePublications->setText(QString::number(numConferencePublications));
            labelNumberOfBooks->setText(QString::number(numBooks));
            labelNumberOfOtherEntries->setText(QString::number(numEntries - numJournalArticles - numConferencePublications - numBooks));
            labelNumberOfComments->setText(QString::number(numComments));
            labelNumberOfMacros->setText(QString::number(numMacros));
        } else {
            labelNumberOfElements->setText(QChar(0x2013));
            labelNumberOfEntries->setText(QChar(0x2013));
            labelNumberOfJournalArticles->setText(QChar(0x2013));
            labelNumberOfConferencePublications->setText(QChar(0x2013));
            labelNumberOfBooks->setText(QChar(0x2013));
            labelNumberOfOtherEntries->setText(QChar(0x2013));
            labelNumberOfComments->setText(QChar(0x2013));
            labelNumberOfMacros->setText(QChar(0x2013));
        }
    }

    void countElement(const QSharedPointer<const Element> &element, int &numEntries, int &numJournalArticles, int &numConferencePublications, int &numBooks, int &numComments, int &numMacros) {
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

    void countElementTypes(int &numElements, int &numEntries, int &numJournalArticles, int &numConferencePublications, int &numBooks, int &numComments, int &numMacros) {
        numElements = numEntries = numJournalArticles = numConferencePublications = numBooks = numComments = numMacros = 0;
        Q_ASSERT_X(file != nullptr, "Statistics::StatisticsPrivate::countElementTypes(..)", "Function was called with file==NULL");

        if (selectionModel != nullptr && selectionModel->selectedRows().count() > 1) {
            /// Use selected items for statistics if selection contains at least two elements
            const auto selectedRows = selectionModel->selectedRows();
            numElements = selectedRows.count();
            for (const QModelIndex &index : selectedRows) {
                const int row = index.row();
                if (row >= 0 && row < file->count())
                    countElement(file->at(row), numEntries, numJournalArticles, numConferencePublications, numBooks, numComments, numMacros);
            }
        } else {
            /// Default/fall-back: use whole file for statistics
            numElements = file->count();
            for (const auto &element : const_cast<const File &>(*file))
                countElement(element, numEntries, numJournalArticles, numConferencePublications, numBooks, numComments, numMacros);
        }
    }
};

Statistics::Statistics(QWidget *parent)
        : QWidget(parent), d(new StatisticsPrivate(this))
{
    d->update();
    connect(&OpenFileInfoManager::instance(), &OpenFileInfoManager::currentChanged, this, &Statistics::update);
}

Statistics::~Statistics()
{
    delete d;
}

void Statistics::setFileView(FileView *fileView)
{
    if (d->fileView != nullptr)
        disconnect(d->fileView, &FileView::selectedElementsChanged, this, &Statistics::update);
    d->fileView = fileView;
    if (d->fileView != nullptr)
        connect(d->fileView, &FileView::selectedElementsChanged, this, &Statistics::update);
    d->update();
}

void Statistics::update()
{
    d->update();
}
