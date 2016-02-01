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
#include <QDir>

#include <KLineEdit>
#include <KDialog>
#include <KLocale>
#include <KDebug>

class AssociatedFilesUI::Private
{
private:
    AssociatedFilesUI *p;

public:
    QLabel *labelGreeting;
    KLineEdit *lineEditSourceUrl;
    QRadioButton *radioNoCopyMove, *radioCopyFile, *radioMoveFile;
    QLabel *labelMoveCopyLocation;
    KLineEdit *lineMoveCopyLocation;
    QGroupBox *groupBoxRename;
    QRadioButton *radioKeepFilename, *radioRenameToEntryId, *radioUserDefinedName;
    KLineEdit *lineEditUserDefinedName;
    QGroupBox *groupBoxPathType;
    QRadioButton *radioRelativePath, *radioAbsolutePath;
    KLineEdit *linePreview;

    QUrl sourceUrl;
    QSharedPointer<Entry> entry;
    QString entryId;
    const File *bibTeXfile;

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
        groupBoxLayout->addSpacing(4);
        labelMoveCopyLocation = new QLabel(i18n("Path and filename of bibliography file:"), groupBox);
        groupBoxLayout->addWidget(labelMoveCopyLocation, 1);
        lineMoveCopyLocation = new KLineEdit(groupBox);
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
        lineEditUserDefinedName = new KLineEdit(groupBoxRename);
        gridLayout->addWidget(lineEditUserDefinedName, 3, 1, 1, 1);
        connect(lineEditUserDefinedName, SIGNAL(textEdited(QString)), p, SLOT(updateUIandPreview()));
        connect(buttonGroup, SIGNAL(buttonClicked(int)), p, SLOT(updateUIandPreview()));
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
        connect(buttonGroup, SIGNAL(buttonClicked(int)), p, SLOT(updateUIandPreview()));
        radioRelativePath->setChecked(true); /// by default

        layout->addSpacing(8);

        label = new QLabel(i18n("Preview of reference to be inserted:"), p);
        layout->addWidget(label);

        linePreview = new KLineEdit(p);
        layout->addWidget(linePreview);
        linePreview->setReadOnly(true);

        layout->addStretch(10);
    }
};

bool AssociatedFilesUI::associateUrl(const QUrl &url, QSharedPointer<Entry> &entry, const File *bibTeXfile, QWidget *parent) {
    QPointer<KDialog> dlg = new KDialog(parent);
    QPointer<AssociatedFilesUI> ui = new AssociatedFilesUI(entry->id(), bibTeXfile, dlg);
    dlg->setMainWidget(ui);

    if (AssociatedFiles::urlIsLocal(url))
        ui->setupForLocalFile(url, entry->id());
    else
        ui->setupForRemoteUrl(url, entry->id());

    const bool accepted = dlg->exec() == KDialog::Accepted;
    bool success = true;
    if (accepted) {
        const QUrl newUrl = AssociatedFiles::copyDocument(url, entry->id(), bibTeXfile, ui->renameOperation(), ui->moveCopyOperation(), dlg, ui->userDefinedFilename());
        success &= !newUrl.isEmpty();
        if (success) {
            const QString referenceString = AssociatedFiles::associateDocumentURL(newUrl, entry, bibTeXfile, ui->pathType());
            success &= !referenceString.isEmpty();
        }
    }

    delete dlg;
    return accepted && success;
}

QString AssociatedFilesUI::associateUrl(const QUrl &url, const QString &entryId, const File *bibTeXfile, QWidget *parent) {
    QPointer<KDialog> dlg = new KDialog(parent);
    QPointer<AssociatedFilesUI> ui = new AssociatedFilesUI(entryId, bibTeXfile, dlg);
    dlg->setMainWidget(ui);

    if (AssociatedFiles::urlIsLocal(url))
        ui->setupForLocalFile(url, entryId);
    else
        ui->setupForRemoteUrl(url, entryId);

    const bool accepted = dlg->exec() == KDialog::Accepted;
    bool success = true;
    QString referenceString;
    if (accepted) {
        const QUrl newUrl = AssociatedFiles::copyDocument(url, entryId, bibTeXfile, ui->renameOperation(), ui->moveCopyOperation(), dlg, ui->userDefinedFilename());
        success &= !newUrl.isEmpty();
        if (success) {
            referenceString = AssociatedFiles::associateDocumentURL(newUrl, bibTeXfile, ui->pathType());
            success &= !referenceString.isEmpty();
        }
    }

    delete dlg;
    return accepted && success ? referenceString : QString();
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
        return AssociatedFiles::roEntryId;
    else if (d->radioKeepFilename->isChecked() || d->lineEditUserDefinedName->text().isEmpty())
        return AssociatedFiles::roKeepName;
    else
        return AssociatedFiles::roUserDefined;
}

AssociatedFiles::MoveCopyOperation AssociatedFilesUI::moveCopyOperation() const {
    if (d->radioNoCopyMove->isChecked()) return AssociatedFiles::mcoNoCopyMove;
    else if (d->radioMoveFile->isChecked()) return AssociatedFiles::mcoMove;
    else return AssociatedFiles::mcoCopy;
}

AssociatedFiles::PathType AssociatedFilesUI::pathType() const {
    return d->radioAbsolutePath->isChecked() ? AssociatedFiles::ptAbsolute : AssociatedFiles::ptRelative;
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
    kDebug() << "d->bibTeXfile != NULL " << (d->bibTeXfile != NULL);
    kDebug() << "!d->sourceUrl.isEmpty() " << (!d->sourceUrl.isEmpty());
    kDebug() << "entryId " << entryId;

    if (entryId.isEmpty()) {
        d->radioRenameToEntryId->setEnabled(false);
        d->radioKeepFilename->setChecked(true);
    } else
        d->radioRenameToEntryId->setEnabled(true);
    if (d->bibTeXfile == NULL || !d->bibTeXfile->hasProperty(File::Url)) {
        d->radioRelativePath->setEnabled(false);
        d->radioAbsolutePath->setChecked(true);
        d->labelMoveCopyLocation->hide();
        d->lineMoveCopyLocation->hide();
    } else {
        d->radioRelativePath->setEnabled(true);
        d->labelMoveCopyLocation->show();
        d->lineMoveCopyLocation->show();
        d->lineMoveCopyLocation->setText(d->bibTeXfile->property(File::Url).toUrl().path());
    }

    if (d->bibTeXfile != NULL && !d->sourceUrl.isEmpty() && !entryId.isEmpty()) {
        const QUrl newUrl = AssociatedFiles::copyDocument(d->sourceUrl, entryId, d->bibTeXfile, renameOperation(), moveCopyOperation(), NULL, d->lineEditUserDefinedName->text(), true);
        kDebug() << "newUrl= " << newUrl.toString();
        if (!newUrl.isEmpty())
            preview = AssociatedFiles::associateDocumentURL(newUrl,  d->bibTeXfile, pathType());
    }
    kDebug() << "preview= " << preview;
    d->linePreview->setText(preview);

    d->groupBoxRename->setEnabled(!d->radioNoCopyMove->isChecked());
}

void AssociatedFilesUI::setupForRemoteUrl(const QUrl &url, const QString &entryId) {
    d->sourceUrl = url;
    d->lineEditSourceUrl->setText(url.toString());
    if (entryId.isEmpty()) {
        d->labelGreeting->setText(i18n("The following remote document is about to be associated with the current entry:"));
        d->radioRenameToEntryId->setText(i18n("Rename after entry's id"));
    } else {
        d->labelGreeting->setText(i18n("The following remote document is about to be associated with the entry '%1':", entryId));
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
        d->labelGreeting->setText(i18n("The following local document is about to be associated with the entry '%1':", entryId));
        d->radioRenameToEntryId->setText(i18n("Rename after entry's id: '%1'", entryId));
    }
    updateUIandPreview();
}
