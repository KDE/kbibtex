/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2020 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "associatedfilesui.h"

#include <QLayout>
#include <QLabel>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>
#include <QPointer>

#include <KLocalizedString>

class AssociatedFilesUI::Private
{
private:
    AssociatedFilesUI *p;

public:
    QLabel *labelGreeting;
    QLineEdit *lineEditSourceUrl;
    QRadioButton *radioNoCopyMove, *radioCopyFile, *radioMoveFile;
    QLabel *labelMoveCopyLocation;
    QLineEdit *lineMoveCopyLocation;
    QGroupBox *groupBoxRename;
    QRadioButton *radioKeepFilename, *radioRenameToEntryId, *radioUserDefinedName;
    QLineEdit *lineEditUserDefinedName;
    QGroupBox *groupBoxPathType;
    QRadioButton *radioRelativePath, *radioAbsolutePath;
    QLineEdit *linePreview;

    QUrl sourceUrl;
    QSharedPointer<Entry> entry;
    QString entryId;
    const File *bibTeXfile;

    Private(AssociatedFilesUI *parent)
            : p(parent), entry(QSharedPointer<Entry>()), bibTeXfile(nullptr) {
        setupGUI();
    }

    void setupGUI() {
        QBoxLayout *layout = new QVBoxLayout(p);

        labelGreeting = new QLabel(p);
        layout->addWidget(labelGreeting);
        labelGreeting->setWordWrap(true);

        lineEditSourceUrl = new QLineEdit(p);
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
#if QT_VERSION >= 0x050f00
        connect(buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::idClicked), p, &AssociatedFilesUI::updateUIandPreview);
#else // QT_VERSION < 0x050f00
        connect(buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), p, &AssociatedFilesUI::updateUIandPreview);
#endif // QT_VERSION >= 0x050f00
        radioNoCopyMove->setChecked(true); /// by default
        groupBoxLayout->addSpacing(4);
        labelMoveCopyLocation = new QLabel(i18n("Path and filename of bibliography file:"), groupBox);
        groupBoxLayout->addWidget(labelMoveCopyLocation, 1);
        lineMoveCopyLocation = new QLineEdit(groupBox);
        lineMoveCopyLocation->setReadOnly(true);
        groupBoxLayout->addWidget(lineMoveCopyLocation, 1);

        groupBoxRename = new QGroupBox(i18n("Rename Document?"), p);
        layout->addWidget(groupBoxRename);
        QGridLayout *gridLayout = new QGridLayout(groupBoxRename);
        gridLayout->setColumnMinimumWidth(0, 16);
        gridLayout->setColumnStretch(0, 0);
        gridLayout->setColumnStretch(1, 1);
        buttonGroup = new QButtonGroup(groupBoxRename);
        radioKeepFilename = new QRadioButton(i18n("Keep document's original filename"), groupBoxRename);
        gridLayout->addWidget(radioKeepFilename, 0, 0, 1, 2);
        buttonGroup->addButton(radioKeepFilename);
        radioRenameToEntryId = new QRadioButton(groupBoxRename);
        gridLayout->addWidget(radioRenameToEntryId, 1, 0, 1, 2);
        buttonGroup->addButton(radioRenameToEntryId);
        radioUserDefinedName = new QRadioButton(i18n("User-defined name:"), groupBoxRename);
        gridLayout->addWidget(radioUserDefinedName, 2, 0, 1, 2);
        buttonGroup->addButton(radioUserDefinedName);
        lineEditUserDefinedName = new QLineEdit(groupBoxRename);
        gridLayout->addWidget(lineEditUserDefinedName, 3, 1, 1, 1);
        connect(lineEditUserDefinedName, &QLineEdit::textEdited, p, &AssociatedFilesUI::updateUIandPreview);
#if QT_VERSION >= 0x050f00
        connect(buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::idClicked), p, &AssociatedFilesUI::updateUIandPreview);
#else // QT_VERSION < 0x050f00
        connect(buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), p, &AssociatedFilesUI::updateUIandPreview);
#endif // QT_VERSION >= 0x050f00
        radioRenameToEntryId->setChecked(true); /// by default

        groupBoxPathType = new QGroupBox(i18n("Path as Inserted into Entry"), p);
        buttonGroup = new QButtonGroup(groupBoxPathType);
        layout->addWidget(groupBoxPathType);
        groupBoxLayout = new QVBoxLayout(groupBoxPathType);
        radioRelativePath = new QRadioButton(i18n("Relative Path"), groupBoxPathType);
        groupBoxLayout->addWidget(radioRelativePath);
        buttonGroup->addButton(radioRelativePath);
        radioAbsolutePath = new QRadioButton(i18n("Absolute Path"), groupBoxPathType);
        groupBoxLayout->addWidget(radioAbsolutePath);
        buttonGroup->addButton(radioAbsolutePath);
#if QT_VERSION >= 0x050f00
        connect(buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::idClicked), p, &AssociatedFilesUI::updateUIandPreview);
#else // QT_VERSION < 0x050f00
        connect(buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), p, &AssociatedFilesUI::updateUIandPreview);
#endif // QT_VERSION >= 0x050f00
        radioRelativePath->setChecked(true); /// by default

        layout->addSpacing(8);

        label = new QLabel(i18n("Preview of reference to be inserted:"), p);
        layout->addWidget(label);

        linePreview = new QLineEdit(p);
        layout->addWidget(linePreview);
        linePreview->setReadOnly(true);

        layout->addStretch(10);
    }
};

QString AssociatedFilesUI::associateUrl(const QUrl &url, QSharedPointer<Entry> &entry, const File *bibTeXfile, const bool doInsertUrl, QWidget *parent) {
    QPointer<QDialog> dlg = new QDialog(parent);
    dlg->setWindowTitle(i18nc("@title:window", "Associate Document with Entry"));
    QBoxLayout *layout = new QVBoxLayout(dlg);
    QPointer<AssociatedFilesUI> ui = new AssociatedFilesUI(entry->id(), bibTeXfile, dlg);
    layout->addWidget(ui);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dlg);
    layout->addWidget(buttonBox);
    dlg->setLayout(layout);

    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg.data(), &QDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, dlg.data(), &QDialog::reject);

    if (url.isLocalFile())
        ui->setupForLocalFile(url, entry->id());
    else
        ui->setupForRemoteUrl(url, entry->id());

    const bool accepted = dlg->exec() == QDialog::Accepted;
    bool success = false;
    QString referenceString;
    if (accepted) {
        const QUrl newUrl = AssociatedFiles::copyDocument(url, entry->id(), bibTeXfile, ui->renameOperation(), ui->moveCopyOperation(), dlg, ui->userDefinedFilename());
        success = newUrl.isValid();
        if (success) {
            referenceString = doInsertUrl ? AssociatedFiles::insertUrl(newUrl, entry, bibTeXfile, ui->pathType()) : AssociatedFiles::computeAssociateString(newUrl, bibTeXfile, ui->pathType());
            success &= !referenceString.isEmpty();
        }
    }

    delete dlg;
    return success ? referenceString : QString();
}

AssociatedFilesUI::AssociatedFilesUI(const QString &entryId, const File *bibTeXfile, QWidget *parent)
        : QWidget(parent), d(new AssociatedFilesUI::Private(this)) {
    d->entryId = entryId;
    d->bibTeXfile = bibTeXfile;
}

AssociatedFilesUI::~AssociatedFilesUI()
{
    delete d;
}

AssociatedFiles::RenameOperation AssociatedFilesUI::renameOperation() const {
    if (d->radioRenameToEntryId->isChecked())
        return AssociatedFiles::RenameOperation::EntryId;
    else if (d->radioKeepFilename->isChecked() || d->lineEditUserDefinedName->text().isEmpty())
        return AssociatedFiles::RenameOperation::KeepName;
    else
        return AssociatedFiles::RenameOperation::UserDefined;
}

AssociatedFiles::MoveCopyOperation AssociatedFilesUI::moveCopyOperation() const {
    if (d->radioNoCopyMove->isChecked()) return AssociatedFiles::MoveCopyOperation::None;
    else if (d->radioMoveFile->isChecked()) return AssociatedFiles::MoveCopyOperation::Move;
    else return AssociatedFiles::MoveCopyOperation::Copy;
}

AssociatedFiles::PathType AssociatedFilesUI::pathType() const {
    return d->radioAbsolutePath->isChecked() ? AssociatedFiles::PathType::Absolute : AssociatedFiles::PathType::Relative;
}

QString AssociatedFilesUI::userDefinedFilename() const {
    QString text = d->lineEditUserDefinedName->text();
    const int p = qMax(text.lastIndexOf(QLatin1Char('/')), text.lastIndexOf(QDir::separator()));
    if (p > 0) text = text.mid(p + 1);
    return text;
}

void AssociatedFilesUI::updateUIandPreview() {
    QString preview = i18n("No preview available");
    const QString entryId = d->entryId.isEmpty() && !d->entry.isNull() ? d->entry->id() : d->entryId;

    const QUrl bibTeXfileUrl = d->bibTeXfile != nullptr && d->bibTeXfile->hasProperty(File::Url) ? d->bibTeXfile->property(File::Url).toUrl() : QUrl();

    if (entryId.isEmpty()) {
        /// If current entry has no identifier, then renaming after entry id is not possible
        d->radioRenameToEntryId->setEnabled(false);
        /// ... and the current filename should be kept
        d->radioKeepFilename->setChecked(true);
    } else
        /// But if the entry has an identifier, prefer to rename after it
        d->radioRenameToEntryId->setEnabled(true);
    if (!bibTeXfileUrl.isValid()) {
        /// If current file has no URL, e.g. because it hasn't been saved,
        /// no relative path can be assembled, i.e. only an absolute one is possible
        d->radioRelativePath->setEnabled(false);
        d->radioAbsolutePath->setChecked(true);
    } else if (bibTeXfileUrl.isValid() && !d->sourceUrl.isRelative() && (bibTeXfileUrl.scheme() != d->sourceUrl.scheme() || (bibTeXfileUrl.scheme() == d->sourceUrl.scheme() && bibTeXfileUrl.host() != d->sourceUrl.host()))) {
        /// If URL to be associated is not relative, i.e. has a scheme and this scheme is
        /// different from the current bibliography file's scheme or the scheme is the same
        /// but the host is different, then no relative path is possible
        d->radioRelativePath->setEnabled(false);
        d->radioAbsolutePath->setChecked(true);
    } else {
        /// If the file has a valid URL, prefer to use relative paths for association
        d->radioRelativePath->setEnabled(true);
    }
    if (bibTeXfileUrl.isValid()) {
        /// Show the URL of the current bibliography file
        d->lineMoveCopyLocation->setText(bibTeXfileUrl.path());
        d->lineMoveCopyLocation->setToolTip(d->lineMoveCopyLocation->text());
        d->labelMoveCopyLocation->show();
        d->lineMoveCopyLocation->show();
    } else {
        d->labelMoveCopyLocation->hide();
        d->lineMoveCopyLocation->hide();
    }
    /// Renaming is only possible if remote file is either copied or moved
    d->groupBoxRename->setEnabled(!d->radioNoCopyMove->isChecked());
    if (d->radioNoCopyMove->isChecked())
        /// Not moving/copying remote file implies that filename is kept
        d->radioKeepFilename->setChecked(true);

    if (d->radioMoveFile->isChecked() || d->radioCopyFile->isChecked()) {
        /// Assuming that the remote URL is to be copied next to the bibliography file,
        /// compute the destination a.k.a. target URL of the copied file
        const QPair<QUrl, QUrl> newURLs = AssociatedFiles::computeSourceDestinationUrls(d->sourceUrl, entryId, d->bibTeXfile, renameOperation(), d->lineEditUserDefinedName->text());
        if (newURLs.second.isValid())
            preview = AssociatedFiles::computeAssociateString(newURLs.second, d->bibTeXfile, pathType());
    } else if (d->radioNoCopyMove->isChecked()) {
        if (d->sourceUrl.isValid())
            preview = AssociatedFiles::computeAssociateString(d->sourceUrl, d->bibTeXfile, pathType());
    }
    d->linePreview->setText(preview);
}

void AssociatedFilesUI::setupForRemoteUrl(const QUrl &url, const QString &entryId) {
    d->sourceUrl = url;
    d->lineEditSourceUrl->setText(url.toDisplayString());
    if (entryId.isEmpty()) {
        d->labelGreeting->setText(i18n("The following remote document is about to be associated with the current entry:"));
        d->radioRenameToEntryId->setText(i18n("Rename after entry's id"));
    } else {
        d->labelGreeting->setText(i18n("The following remote document is about to be associated with entry '%1':", entryId));
        d->radioRenameToEntryId->setText(i18n("Rename after entry's id: '%1'", entryId));
    }
    updateUIandPreview();
}

void AssociatedFilesUI::setupForLocalFile(const QUrl &url, const QString &entryId) {
    d->sourceUrl = url;
    d->lineEditSourceUrl->setText(url.path());
    if (entryId.isEmpty()) {
        d->labelGreeting->setText(i18n("The following local document is about to be associated with the current entry:"));
        d->radioRenameToEntryId->setText(i18n("Rename after entry's id"));
    } else {
        d->labelGreeting->setText(i18n("The following local document is about to be associated with entry '%1':", entryId));
        d->radioRenameToEntryId->setText(i18n("Rename after entry's id: '%1'", entryId));
    }
    updateUIandPreview();
}
