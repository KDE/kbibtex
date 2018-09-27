/***************************************************************************
 *   Copyright (C) 2004-2018 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#include "fileview.h"

#include <QDropEvent>
#include <QTimer>
#include <QDialog>
#include <QDialogButtonBox>
#include <QBoxLayout>
#include <QPushButton>

#include <KLocalizedString>
#include <KMessageBox>
#include <KGuiItem>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KWindowConfig>
#include <KStandardGuiItem>

#include "elementeditor.h"
#include "entry.h"
#include "macro.h"
#include "models/filemodel.h"
#include "fileexporterbibtex.h"
#include "valuelistmodel.h"

#include "logging_gui.h"

/**
 * Specialized dialog for element editing. It will check if the used
 * element editor widget has unapplied changes and ask the user if
 * he/she actually wants to discard those changes before closing this
 * dialog.
 *
 * @author Thomas Fischer
 */
class ElementEditorDialog : public QDialog
{
    Q_OBJECT

private:
    ElementEditor *elementEditor;
    static const QString configGroupNameWindowSize;
    KConfigGroup configGroup;

public:
    ElementEditorDialog(QWidget *parent)
            : QDialog(parent), elementEditor(nullptr) {
        /// restore window size
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        configGroup = KConfigGroup(config, configGroupNameWindowSize);
        KWindowConfig::restoreWindowSize(windowHandle(), configGroup);

        setLayout(new QVBoxLayout(parent));
    }

    /**
     * Store element editor widget for future reference.
     */
    void setElementEditor(ElementEditor *elementEditor) {
        this->elementEditor = elementEditor;
        QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(layout());
        boxLayout->addWidget(this->elementEditor);
    }

protected:
    void closeEvent(QCloseEvent *e) override {
        /// strangely enough, close events have always to be rejected ...
        e->setAccepted(false);
        QDialog::closeEvent(e);
    }

private:
    bool allowedToClose() {
        /// save window size
        KWindowConfig::saveWindowSize(windowHandle(), configGroup);

        /// if there unapplied changes in the editor widget ...
        /// ... ask user for consent to discard changes ...
        /// only the allow to close this dialog
        return !elementEditor->elementUnapplied() || KMessageBox::warningContinueCancel(this, i18n("The current entry has been modified. Do you want do discard your changes?"), i18n("Discard changes?"), KStandardGuiItem::discard(), KGuiItem(i18n("Continue Editing"), QStringLiteral("edit-rename"))) == KMessageBox::Continue;
    }
};

const QString ElementEditorDialog::configGroupNameWindowSize = QStringLiteral("ElementEditorDialog");

FileView::FileView(const QString &name, QWidget *parent)
        : BasicFileView(name, parent), m_isReadOnly(false), m_current(QSharedPointer<Element>()), m_filterBar(nullptr), m_lastEditorPage(nullptr), m_elementEditorDialog(nullptr), m_elementEditor(nullptr), m_dbb(nullptr)
{
    connect(this, &FileView::doubleClicked, this, &FileView::itemActivated);
}

void FileView::viewCurrentElement()
{
    viewElement(currentElement());
}

void FileView::viewElement(const QSharedPointer<Element> element)
{
    prepareEditorDialog(DialogTypeView);
    FileModel *model = fileModel();
    File *bibliographyFile = model != nullptr ? model->bibliographyFile() : nullptr;
    m_elementEditor->setElement(element, bibliographyFile);

    m_elementEditor->setCurrentPage(m_lastEditorPage);
    m_elementEditorDialog->exec();
    m_lastEditorPage = m_elementEditor->currentPage();
}

void FileView::editCurrentElement()
{
    editElement(currentElement());
}

bool FileView::editElement(QSharedPointer<Element> element)
{
    prepareEditorDialog(DialogTypeEdit);
    FileModel *model = fileModel();
    File *bibliographyFile = model != nullptr ? model->bibliographyFile() : nullptr;
    m_elementEditor->setElement(element, bibliographyFile);

    m_elementEditor->setCurrentPage(m_lastEditorPage);
    m_elementEditorDialog->exec();
    m_lastEditorPage = m_elementEditor->currentPage();

    if (!isReadOnly()) {
        bool changed = m_elementEditor->elementChanged();
        if (changed) {
            FileModel *model = fileModel();
            const File *bibliographyFile = model != nullptr ? model->bibliographyFile() : nullptr;
            emit currentElementChanged(currentElement(), bibliographyFile);
            emit selectedElementsChanged();
            emit modified(true);
        }
        return changed;
    } else
        return false;
}

const QList<QSharedPointer<Element> > &FileView::selectedElements() const
{
    return m_selection;
}

void FileView::setSelectedElement(QSharedPointer<Element> element)
{
    m_selection.clear();
    m_selection << element;

    QItemSelectionModel *selModel = selectionModel();
    selModel->clear();
    FileModel *model = fileModel();
    const int row = model != nullptr ? model->row(element) : -1;
    const QModelIndex sourceIdx = row >= 0 && model != nullptr ? model->index(row, 0) : QModelIndex();
    const QModelIndex idx = sortFilterProxyModel()->mapFromSource(sourceIdx);
    selModel->setCurrentIndex(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

const QSharedPointer<Element> FileView::currentElement() const
{
    return m_current;
}

QSharedPointer<Element> FileView::currentElement()
{
    return m_current;
}

QSharedPointer<Element> FileView::elementAt(const QModelIndex &index)
{
    FileModel *model = fileModel();
    return model != nullptr ? model->element(sortFilterProxyModel()->mapToSource(index).row()) : QSharedPointer<Element>();
}

void FileView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous); // FIXME necessary?

    m_current = elementAt(current);
    FileModel *model = fileModel();
    if (model != nullptr)
        emit currentElementChanged(m_current, model->bibliographyFile());
}

void FileView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    const QModelIndexList selectedSet = selected.indexes();
    for (const QModelIndex &index : selectedSet) {
        if (index.column() != 0) continue; ///< consider only column-0 indices to avoid duplicate elements
        m_selection.append(elementAt(index));
    }
    if (m_current.isNull() && !selectedSet.isEmpty())
        m_current = elementAt(selectedSet.first());

    const QModelIndexList deselectedSet = deselected.indexes();
    for (const QModelIndex &index : deselectedSet) {
        if (index.column() != 0) continue; ///< consider only column-0 indices to avoid duplicate elements
        m_selection.removeOne(elementAt(index));
    }

    emit selectedElementsChanged();
}

void FileView::selectionDelete()
{
    const QModelIndexList mil = selectionModel()->selectedRows();
    QList<int> rows;
    rows.reserve(mil.size());
    for (const QModelIndex &idx : mil)
        rows << sortFilterProxyModel()->mapToSource(idx).row();

    FileModel *model = fileModel();
    if (model != nullptr) model->removeRowList(rows);

    emit modified(true);
}

/// FIXME the existence of this function is basically just one big hack
void FileView::externalModification()
{
    emit modified(true);
}

void FileView::setReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
}

bool FileView::isReadOnly() const
{
    return m_isReadOnly;
}

ValueListModel *FileView::valueListModel(const QString &field)
{
    FileModel *model = fileModel();
    if (model != nullptr) {
        ValueListModel *result = new ValueListModel(model->bibliographyFile(), field, this);
        /// Keep track of external changes through modifications in this ValueListModel instance
        connect(result, &ValueListModel::dataChanged, this, &FileView::externalModification);
        return result;
    }

    return nullptr;
}

void FileView::setFilterBar(FilterBar *filterBar)
{
    m_filterBar = filterBar;
}

void FileView::setFilterBarFilter(const SortFilterFileModel::FilterQuery &fq)
{
    if (m_filterBar != nullptr)
        m_filterBar->setFilter(fq);
}

void FileView::mouseMoveEvent(QMouseEvent *event)
{
    emit editorMouseEvent(event);
}

void FileView::dragEnterEvent(QDragEnterEvent *event)
{
    emit editorDragEnterEvent(event);
}

void FileView::dropEvent(QDropEvent *event)
{
    if (event->source() != this)
        emit editorDropEvent(event);
}

void FileView::dragMoveEvent(QDragMoveEvent *event)
{
    emit editorDragMoveEvent(event);
}

void FileView::itemActivated(const QModelIndex &index)
{
    emit elementExecuted(elementAt(index));
}

void FileView::prepareEditorDialog(DialogType dialogType)
{
    if (dialogType != DialogTypeView && isReadOnly()) {
        qCWarning(LOG_KBIBTEX_GUI) << "In read-only mode, you may only view elements, not edit them";
        dialogType = DialogTypeView;
    }

    /// Create both the dialog window and the editing widget only once
    if (m_elementEditorDialog == nullptr)
        m_elementEditorDialog = new ElementEditorDialog(this);
    if (m_elementEditor == nullptr) {
        m_elementEditor = new ElementEditor(false, m_elementEditorDialog);
        m_elementEditorDialog->setElementEditor(m_elementEditor);
    }
    if (m_dbb != nullptr) {
        delete m_dbb;
        m_dbb = nullptr;
    }

    if (dialogType == DialogTypeView) {
        /// View mode, as use in read-only situations
        m_elementEditor->setReadOnly(true);
        m_elementEditorDialog->setWindowTitle(i18n("View Element"));
        m_dbb = new QDialogButtonBox(QDialogButtonBox::Close, m_elementEditorDialog);
        QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(m_elementEditorDialog->layout());
        boxLayout->addWidget(m_dbb);
        connect(m_dbb, &QDialogButtonBox::clicked, this, &FileView::dialogButtonClicked);
    } else if (dialogType == DialogTypeEdit) {
        /// Edit mode, used in normal operations
        m_elementEditor->setReadOnly(false);
        m_elementEditorDialog->setWindowTitle(i18n("Edit Element"));
        m_dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Reset, m_elementEditorDialog);
        QBoxLayout *boxLayout = qobject_cast<QBoxLayout *>(m_elementEditorDialog->layout());
        boxLayout->addWidget(m_dbb);
        m_dbb->button(QDialogButtonBox::Apply)->setEnabled(false);

        /// Establish signal-slot connections for modification/editing events
        connect(m_elementEditor, &ElementEditor::modified, m_dbb->button(QDialogButtonBox::Apply), &QPushButton::setEnabled);
        connect(m_dbb, &QDialogButtonBox::clicked, this, &FileView::dialogButtonClicked);
    }
}

void FileView::dialogButtonClicked(QAbstractButton *button) {
    switch (m_dbb->standardButton(button)) {
    case QDialogButtonBox::Ok:
        m_elementEditor->apply();
        m_elementEditorDialog->accept();
        break;
    case QDialogButtonBox::Apply:
        m_elementEditor->apply();
        break;
    case QDialogButtonBox::Close: ///< fall-through is intentional
    case QDialogButtonBox::Cancel:
        m_elementEditorDialog->reject();
        break;
    case QDialogButtonBox::Reset:
        m_elementEditor->reset();
        break;
    default:
        qCWarning(LOG_KBIBTEX_GUI) << "Default case should never get triggered in FileView::dialogButtonClicked";
    }
}

#include "fileview.moc"
