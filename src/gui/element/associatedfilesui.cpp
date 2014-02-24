/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "associatedfilesui.h"

#include <QLayout>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>

#include <KLineEdit>
#include <KDialog>
#include <KLocale>
#include <KDebug>

class AssociatedFilesUI::Private
{
private:
    AssociatedFilesUI *p;

public:
    QLabel *labelGreeting, *labelFollowingOperations;
    KLineEdit *lineEditSourceUrl;
    QRadioButton *radioNoCopyMove, *radioCopyFile, *radioMoveFile;
    QGroupBox *groupBoxRename;
    QRadioButton *radioKeepFilename, *radioRenameToEntryId;
    QGroupBox *groupBoxPathType;
    QRadioButton *radioRelativePath, *radioAbsolutePath;
    KLineEdit *linePreview;

    QUrl sourceUrl;
    QSharedPointer<Entry> entry;
    File *bibTeXfile;

    Private(AssociatedFilesUI *parent)
            : p(parent), entry(QSharedPointer<Entry>()), bibTeXfile(NULL) {
        setupGUI();
    }

    void setupGUI() {
        QBoxLayout *layout = new QVBoxLayout(p);

        labelGreeting = new QLabel(p);
        layout->addWidget(labelGreeting);
        labelGreeting->setWordWrap(true);

        lineEditSourceUrl = new KLineEdit(p);
        layout->addWidget(lineEditSourceUrl);
        lineEditSourceUrl->setReadOnly(true);

        layout->addSpacing(8);

        QLabel *label = new QLabel(i18n("The following operations can be performed when associating the document with the entry:"), p);
        layout->addWidget(label);
        label->setWordWrap(true);

        QGroupBox *groupBox = new QGroupBox(i18n("File operation"), p);
        layout->addWidget(groupBox);
        QBoxLayout *groupBoxLayout = new QVBoxLayout(groupBox);
        QButtonGroup *buttonGroup = new QButtonGroup(groupBox);
        radioNoCopyMove = new QRadioButton(i18n("Do not copy or move document, only insert reference to it"), groupBox);
        groupBoxLayout->addWidget(radioNoCopyMove);
        buttonGroup->addButton(radioNoCopyMove);
        radioCopyFile = new QRadioButton(i18n("Copy document next to bibliography file"), groupBox);
        groupBoxLayout->addWidget(radioCopyFile);
        buttonGroup->addButton(radioCopyFile);
        radioMoveFile = new QRadioButton(i18n("Move document next to bibliography file"), groupBox);
        groupBoxLayout->addWidget(radioMoveFile);
        buttonGroup->addButton(radioMoveFile);
        connect(buttonGroup, SIGNAL(buttonClicked(int)), p, SLOT(updateUIandPreview()));
        radioNoCopyMove->setChecked(true); /// by default

        groupBoxRename = new QGroupBox(i18n("Rename Document?"), p);
        layout->addWidget(groupBoxRename);
        groupBoxLayout = new QVBoxLayout(groupBoxRename);
        buttonGroup = new QButtonGroup(groupBoxRename);
        radioKeepFilename = new QRadioButton(i18n("Keep document's original filename"), groupBoxRename);
        groupBoxLayout->addWidget(radioKeepFilename);
        buttonGroup->addButton(radioKeepFilename);
        radioRenameToEntryId = new QRadioButton(i18n("Rename after entry's id"), groupBoxRename);
        groupBoxLayout->addWidget(radioRenameToEntryId);
        buttonGroup->addButton(radioRenameToEntryId);
        connect(buttonGroup, SIGNAL(buttonClicked(int)), p, SLOT(updateUIandPreview()));
        radioRenameToEntryId->setChecked(true); /// by default

        groupBoxPathType = new QGroupBox(i18n("Path Type"), p);
        layout->addWidget(groupBoxPathType);
        groupBoxLayout = new QVBoxLayout(groupBoxPathType);
        radioRelativePath = new QRadioButton(i18n("Relative Path"), groupBoxPathType);
        groupBoxLayout->addWidget(radioRelativePath);
        radioAbsolutePath = new QRadioButton(i18n("Absolute Path"), groupBoxPathType);
        groupBoxLayout->addWidget(radioAbsolutePath);
        connect(radioAbsolutePath, SIGNAL(clicked()), p, SLOT(updateUIandPreview()));
        radioAbsolutePath->setChecked(true); /// by default

        layout->addSpacing(8);

        label = new QLabel(i18n("Preview of reference to be inserted:"), p);
        layout->addWidget(label);

        linePreview = new KLineEdit(p);
        layout->addWidget(linePreview);
        linePreview->setReadOnly(true);

        layout->addStretch(10);
    }
};

bool AssociatedFilesUI::associateUrl(const QUrl &url, QSharedPointer<Entry> &entry, File *bibTeXfile, QWidget *parent) {
    QPointer<KDialog> dlg = new KDialog(parent);
    QPointer<AssociatedFilesUI> ui = new AssociatedFilesUI(entry, bibTeXfile, dlg);
    dlg->setMainWidget(ui);

    if (AssociatedFiles::urlIsLocal(url))
        ui->setupForLocalFile(url, entry->id());
    else
        ui->setupForRemoteUrl(url, entry->id());

    const bool accepted = dlg->exec() == KDialog::Accepted;
    bool success = true;
    if (accepted) {
        const QUrl newUrl = AssociatedFiles::copyDocument(url, entry, bibTeXfile, ui->renameOperation(), ui->moveCopyOperation(), dlg);
        success &= !newUrl.isEmpty();
        if (success) {
            const QString referenceString = AssociatedFiles::associateDocumentURL(newUrl, entry, bibTeXfile, ui->pathType());
            success &= !referenceString.isEmpty();
        }
    }

    dlg->deleteLater();
    return accepted && success;
}

AssociatedFilesUI::AssociatedFilesUI(QSharedPointer<Entry> &entry, File *bibTeXfile, QWidget *parent)
        : QWidget(parent), d(new AssociatedFilesUI::Private(this)) {
    d->entry = entry;
    d->bibTeXfile = bibTeXfile;
}

AssociatedFiles::RenameOperation AssociatedFilesUI::renameOperation() const {
    return d->radioRenameToEntryId->isChecked() ? AssociatedFiles::roEntryId : AssociatedFiles::roKeepName;
}
AssociatedFiles::MoveCopyOperation AssociatedFilesUI::moveCopyOperation() const {
    if (d->radioNoCopyMove->isChecked()) return AssociatedFiles::mcoNoCopyMove;
    else if (d->radioMoveFile->isChecked()) return AssociatedFiles::mcoMove;
    else return AssociatedFiles::mcoCopy;
}

AssociatedFiles::PathType AssociatedFilesUI::pathType() const {
    return d->radioAbsolutePath->isChecked() ? AssociatedFiles::ptAbsolute : AssociatedFiles::ptRelative;
}

void AssociatedFilesUI::updateUIandPreview() {
    QString preview = i18n("No preview available");
    kDebug() << "d->bibTeXfile != NULL " << (d->bibTeXfile != NULL);
    kDebug() << "!d->sourceUrl.isEmpty() " << (!d->sourceUrl.isEmpty());
    kDebug() << "!d->entry.isNull() " << (!d->entry.isNull());
    if (d->bibTeXfile != NULL && !d->sourceUrl.isEmpty() && !d->entry.isNull()) {
        const QUrl newUrl = AssociatedFiles::copyDocument(d->sourceUrl, d->entry, d->bibTeXfile, renameOperation(), moveCopyOperation(), NULL, true);
        kDebug() << "newUrl= " << newUrl.toString();
        if (!newUrl.isEmpty())
            preview = AssociatedFiles::associateDocumentURL(newUrl, d->entry, d->bibTeXfile, pathType(), true);
    }
    kDebug() << "preview= " << preview;
    d->linePreview->setText(preview);

    d->groupBoxRename->setEnabled(!d->radioNoCopyMove->isChecked());
}

void AssociatedFilesUI::setupForRemoteUrl(const QUrl &url, const QString &entryId) {
    d->sourceUrl = url;
    d->lineEditSourceUrl->setText(url.toString());
    d->labelGreeting->setText(i18n("The following remote document is about to be associated with the entry '%1':", entryId));
    updateUIandPreview();
}

void AssociatedFilesUI::setupForLocalFile(const QUrl &url, const QString &entryId) {
    d->sourceUrl = url;
    d->lineEditSourceUrl->setText(url.path());
    d->labelGreeting->setText(i18n("The following local document is about to be associated with the entry '%1':", entryId));
    updateUIandPreview();
}
