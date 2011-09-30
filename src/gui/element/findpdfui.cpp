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

#include <QGridLayout>
#include <QListView>
#include <QAbstractListModel>
#include <QUrl>
#include <QPainter>
#include <QToolButton>
#include <QApplication>
#include <QAction>
#include <QButtonGroup>
#include <QRadioButton>
#include <QDesktopServices>

#include <KDialog>
#include <KLocale>
#include <KDebug>
#include <KSqueezedTextLabel>
#include <KMenu>
#include <KPushButton>
#include <KMimeType>
#include <KTemporaryFile>
#include <KFileDialog>
#include <KIO/NetAccess>

#include <fieldlistedit.h>
#include "findpdfui.h"

class PDFListModel;

const int posLabel = 0;
const int posViewButton = 1;
const int posRadioNoDownload = 2;
const int posRadioDownload = 3;
const int posRadioURLonly = 4;

const int URLRole = Qt::UserRole + 1234;
const int TempFileNameRole = Qt::UserRole + 1236;
const int DownloadModeRole = Qt::UserRole + 1235;

/// inspired by KNewStuff3's ItemsViewDelegate
PDFItemDelegate::PDFItemDelegate(QListView *itemView, QObject *parent)
        : KWidgetItemDelegate(itemView, parent), m_parent(itemView)
{
    // nothing
}

/// paint the item at index with all its attributes shown
void PDFItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, 0);

    painter->save();

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlightedText().color()));
    } else {
        painter->setPen(QPen(option.palette.text().color()));
    }

    QPixmap icon = index.data(Qt::DecorationRole).value<QPixmap>();
    if (!icon.isNull()) {
        int margin = option.fontMetrics.height() / 3;
        painter->drawPixmap(margin, margin, KIconLoader::SizeMedium, KIconLoader::SizeMedium, icon);
    }

    painter->restore();
}

/// get the list of widgets
QList<QWidget*> PDFItemDelegate::createItemWidgets() const
{
    QList<QWidget*> list;

    KSqueezedTextLabel *label = new KSqueezedTextLabel();
    label->setBackgroundRole(QPalette::NoRole);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    list << label;
    Q_ASSERT(list.count() == posLabel + 1);

    KPushButton *pushButton = new KPushButton(KIcon("application-pdf"), i18n("View"));
    list << pushButton;
    connect(pushButton, SIGNAL(clicked()), this, SLOT(slotViewPDF()));
    Q_ASSERT(list.count() == posViewButton + 1);

    QButtonGroup *bg = new QButtonGroup();
    QRadioButton *radioButton = new QRadioButton(i18n("Ignore"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, SIGNAL(toggled(bool)), this, SLOT(slotRadioNoDownloadToggled(bool)));
    Q_ASSERT(list.count() == posRadioNoDownload + 1);

    radioButton = new QRadioButton(i18n("Download"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, SIGNAL(toggled(bool)), this, SLOT(slotRadioDownloadToggled(bool)));
    Q_ASSERT(list.count() == posRadioDownload + 1);

    radioButton = new QRadioButton(i18n("Keep URL"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, SIGNAL(toggled(bool)), this, SLOT(slotRadioURLonlyToggled(bool)));
    Q_ASSERT(list.count() == posRadioURLonly + 1);

    return list;
}

/// update the widgets
void PDFItemDelegate::updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    if (!index.isValid()) return;

    const PDFListModel *model = qobject_cast<const PDFListModel*>(index.model());
    if (model == NULL) {
        kDebug() << "WARNING - INVALID MODEL!";
        return;
    }

    int margin = option.fontMetrics.height() / 3;
    int buttonHeight = option.fontMetrics.height() * 2;
    int buttonWidth = option.fontMetrics.height() * 8;

    /// setup label
    KSqueezedTextLabel *label = qobject_cast<KSqueezedTextLabel*>(widgets[0]);
    if (label != NULL) {
        label->setText(index.data(Qt::DisplayRole).toString());
        label->move(margin*2 + KIconLoader::SizeMedium, margin);
        label->resize(option.rect.width() - 3*margin - KIconLoader::SizeMedium, option.rect.height() - 3*margin - buttonHeight);
    }

    /// setup the view button
    KPushButton *viewButton = qobject_cast<KPushButton*>(widgets.at(posViewButton));
    if (viewButton != NULL) {
        viewButton->move(margin*2 + KIconLoader::SizeMedium, option.rect.bottom() - margin - buttonHeight);
        viewButton->resize(buttonWidth, buttonHeight);
    }

    for (int i = 0; i < 3; ++i) {
        QRadioButton *radioButton = qobject_cast<QRadioButton*>(widgets.at(posRadioNoDownload + i));
        if (radioButton != NULL) {
            radioButton->move(option.rect.width() - margin - (3 - i)*(buttonWidth + margin), option.rect.bottom() - margin - buttonHeight);
            radioButton->resize(buttonWidth, buttonHeight);
            bool ok = false;
            radioButton->setChecked(i + FindPDF::NoDownload == index.data(DownloadModeRole).toInt(&ok) && ok);
        }
    }
}

QSize PDFItemDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex &) const
{
    QSize size;
    size.setWidth(option.fontMetrics.height() * 14);
    size.setHeight(qMax(option.fontMetrics.height() * 4, (int)KIconLoader::SizeMedium));
    return size;
}

void PDFItemDelegate::slotViewPDF()
{
    QModelIndex index = focusedIndex();

    if (index.isValid()) {
        QString tempfileName = index.data(TempFileNameRole).toString();
        QUrl url = index.data(URLRole).toUrl();
        if (!tempfileName.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(tempfileName));
        } else if (url.isValid()) {
            QDesktopServices::openUrl(url);
        }
    }
}

void PDFItemDelegate::slotRadioNoDownloadToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::NoDownload, DownloadModeRole);
    }
}

void PDFItemDelegate::slotRadioDownloadToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::Download, DownloadModeRole);
    }
}

void PDFItemDelegate::slotRadioURLonlyToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::URLonly, DownloadModeRole);
    }
}

PDFListModel::PDFListModel(QList<FindPDF::ResultItem> &resultList, QObject *parent)
        : QAbstractListModel(parent), m_resultList(resultList)
{
    // nothing
}

int PDFListModel::rowCount(const QModelIndex & parent) const
{
    int count = parent == QModelIndex() ? m_resultList.count() : 0;
    return count;
}

QVariant PDFListModel::data(const QModelIndex &index, int role) const
{
    if (index != QModelIndex() && index.parent() == QModelIndex() && index.row() < m_resultList.count()) {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole)
            return m_resultList[index.row()].url.toString();
        else if (role == URLRole)
            return  m_resultList[index.row()].url;
        else if (role == TempFileNameRole) {
            if (m_resultList[index.row()].tempFilename != NULL)
                return m_resultList[index.row()].tempFilename->fileName();
            else
                return QVariant();
        } else if (role == DownloadModeRole)
            return m_resultList[index.row()].downloadMode;
        else if (role == Qt::DecorationRole) {
            KMimeType::Ptr mimetypePtr = KMimeType::findByUrl(m_resultList[index.row()].url);
            return KIcon(mimetypePtr->iconName() == QLatin1String("application/octet-stream") ? QLatin1String("application/pdf") : mimetypePtr->iconName()).pixmap(KIconLoader::SizeMedium, KIconLoader::SizeMedium);
        } else
            return QVariant();
    }
    return QVariant();
}

bool PDFListModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    if (index != QModelIndex() && index.row() < m_resultList.count() && role == DownloadModeRole) {
        bool ok = false;
        FindPDF::DownloadMode downloadMode = (FindPDF::DownloadMode)value.toInt(&ok);
        kDebug() << "FindPDF::DownloadMode=" << downloadMode << ok;
        if (ok) {
            kDebug()    << "Setting row " << index.row();
            m_resultList[index.row()].downloadMode = downloadMode;
            return true;
        }
    }
    return false;
}

QVariant PDFListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    if (section == 0) {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            return i18n("Result");
        } else
            return QVariant();
    }
    return QVariant();
}


FindPDFUI::FindPDFUI(Entry &entry, QWidget *parent)
        : QWidget(parent), m_findpdf(new FindPDF(this))
{
    createGUI();

    connect(m_findpdf, SIGNAL(finished()), this, SLOT(searchFinished()));
    m_findpdf->search(entry);
}

void FindPDFUI::interactiveFindPDF(Entry &entry, const File &bibtexFile, QWidget *parent)
{
    KDialog dlg(parent);
    FindPDFUI widget(entry, &dlg);
    dlg.setCaption(i18n("Find PDF"));
    dlg.setMainWidget(&widget);
    dlg.enableButtonOk(false);

    connect(&widget, SIGNAL(resultAvailable(bool)), &dlg, SLOT(enableButtonOk(bool)));

    if (dlg.exec() == KDialog::Accepted) {
        widget.apply(entry, bibtexFile);
    }
}

void FindPDFUI::apply(Entry &entry, const File &bibtexFile)
{
    QAbstractItemModel *model = m_listViewResult->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        bool ok = false;
        FindPDF::DownloadMode downloadMode = (FindPDF::DownloadMode)model->data(model->index(i, 0), DownloadModeRole).toInt(&ok);
        if (!ok) {
            kDebug() << "Could not interprete download mode";
            downloadMode = FindPDF::NoDownload;
        }

        QUrl url = model->data(model->index(i, 0), URLRole).toUrl();
        QString tempfileName = model->data(model->index(i, 0), TempFileNameRole).toString();

        if (downloadMode == FindPDF::URLonly && url.isValid()) {
            bool alreadyContained = false;
            for (QMap<QString, Value>::ConstIterator it = entry.constBegin(); !alreadyContained && it != entry.constEnd();++it)
                alreadyContained |= it.key().toLower().startsWith(Entry::ftUrl) && PlainTextValue::text(it.value()) == url.toString();
            if (!alreadyContained) {
                Value value;
                value.append(new VerbatimText(url.toString()));
                if (!entry.contains(Entry::ftUrl))
                    entry.insert(Entry::ftUrl, value);
                else
                    for (int i = 2; i < 256; ++i) {
                        const QString keyName = QString("%1%2").arg(Entry::ftUrl).arg(i);
                        if (!entry.contains(keyName)) {
                            entry.insert(keyName, value);
                            break;
                        }
                    }
            }
        } else if (downloadMode == FindPDF::Download && !tempfileName.isEmpty()) {
            KUrl startUrl = bibtexFile.property(File::Url, QUrl()).toUrl();
            QString localFilename = KFileDialog::getSaveFileName(KUrl::fromLocalFile(startUrl.directory()), QLatin1String("application/pdf"), this, i18n("Save URL \"%1\"", url.toString()));
            localFilename = UrlListEdit::askRelativeOrStaticFilename(this, localFilename, startUrl);

            if (!localFilename.isEmpty()) {
                KIO::NetAccess::file_copy(KUrl::fromLocalFile(tempfileName), KUrl::fromLocalFile(localFilename), this);

                bool alreadyContained = false;
                for (QMap<QString, Value>::ConstIterator it = entry.constBegin(); !alreadyContained && it != entry.constEnd();++it)
                    alreadyContained |= (it.key().toLower().startsWith(Entry::ftLocalFile) || it.key().toLower().startsWith(Entry::ftUrl)) && PlainTextValue::text(it.value()) == url.toString();
                if (!alreadyContained) {
                    Value value;
                    value.append(new VerbatimText(localFilename));
                    if (!entry.contains(Entry::ftLocalFile))
                        entry.insert(Entry::ftLocalFile, value);
                    else
                        for (int i = 2; i < 256; ++i) {
                            const QString keyName = QString("%1%2").arg(Entry::ftLocalFile).arg(i);
                            if (!entry.contains(keyName)) {
                                entry.insert(keyName, value);
                                break;
                            }
                        }
                }
            }
        }
    }
}

void FindPDFUI::createGUI()
{
    QGridLayout *layout = new QGridLayout(this);

    m_listViewResult = new QListView(this);
    layout->addWidget(m_listViewResult, 0, 0, 1, 1);
    m_listViewResult->setEnabled(false);
    m_listViewResult->setMinimumSize(fontMetrics().height()*40, fontMetrics().height()*20);

    static_cast<QWidget*>(parent())->setCursor(Qt::WaitCursor);
}

void FindPDFUI::searchFinished()
{
    m_resultList = m_findpdf->results();
    m_listViewResult->setModel(new PDFListModel(m_resultList, m_listViewResult));
    m_listViewResult->setItemDelegate(new PDFItemDelegate(m_listViewResult, m_listViewResult));
    m_listViewResult->setEnabled(true);
    m_listViewResult->reset();
    kDebug() << "row count= " << m_listViewResult->model()->rowCount(QModelIndex());

    static_cast<QWidget*>(parent())->unsetCursor();
    emit resultAvailable(true);
}
