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

#include "findpdfui.h"
#include "findpdfui_p.h"

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
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTemporaryFile>

#include <KLocalizedString>
#include <KIconLoader>
#include <KSqueezedTextLabel>
#include <KRun>
#include <kio_version.h>

#include "fileinfo.h"
#include "fieldlistedit.h"
#include "logging_gui.h"

class PDFListModel;

const int posLabelUrl = 0;
const int posLabelPreview = 1;
const int posViewButton = 2;
const int posRadioNoDownload = 3;
const int posRadioDownload = 4;
const int posRadioURLonly = 5;

/// inspired by KNewStuff3's ItemsViewDelegate
PDFItemDelegate::PDFItemDelegate(QListView *itemView, QObject *parent)
        : KWidgetItemDelegate(itemView, parent), m_parent(itemView)
{
    /// nothing
}

void PDFItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, nullptr);

    painter->save();

    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(option.palette.highlightedText().color()));
    } else {
        painter->setPen(QPen(option.palette.text().color()));
    }

    /// draw icon based on mime-type
    QPixmap icon = index.data(Qt::DecorationRole).value<QPixmap>();
    if (!icon.isNull()) {
        int margin = option.fontMetrics.height() / 3;
        painter->drawPixmap(margin, margin + option.rect.top(), KIconLoader::SizeMedium, KIconLoader::SizeMedium, icon);
    }

    painter->restore();
}

QList<QWidget *> PDFItemDelegate::createItemWidgets(const QModelIndex &index) const
{
    Q_UNUSED(index) // FIXME really of no use?
    QList<QWidget *> list;

    /// first, the label with shows the found PDF file's origin (URL)
    KSqueezedTextLabel *label = new KSqueezedTextLabel();
    label->setBackgroundRole(QPalette::NoRole);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    list << label;
    Q_ASSERT_X(list.count() == posLabelUrl + 1, "QList<QWidget *> PDFItemDelegate::createItemWidgets() const", "list.count() != posLabelUrl + 1");

    /// a label with shows either the PDF's title or a text snipplet
    QLabel *previewLabel = new QLabel();
    previewLabel->setBackgroundRole(QPalette::NoRole);
    previewLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    list << previewLabel;
    Q_ASSERT_X(list.count() == posLabelPreview + 1, "QList<QWidget *> PDFItemDelegate::createItemWidgets() const", "list.count() != posLabelPreview + 1");

    /// add a push button to view the PDF file
    QPushButton *pushButton = new QPushButton(QIcon::fromTheme(QStringLiteral("application-pdf")), i18n("View"));
    list << pushButton;
    connect(pushButton, &QPushButton::clicked, this, &PDFItemDelegate::slotViewPDF);
    Q_ASSERT_X(list.count() == posViewButton + 1, "QList<QWidget *> PDFItemDelegate::createItemWidgets() const", "list.count() != posViewButton + 1");

    /// a button group to choose what to do with this particular PDF file
    QButtonGroup *bg = new QButtonGroup();

    /// button group's first choice: ignore file (discard it)
    QRadioButton *radioButton = new QRadioButton(i18n("Ignore"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, &QRadioButton::toggled, this, &PDFItemDelegate::slotRadioNoDownloadToggled);
    Q_ASSERT_X(list.count() == posRadioNoDownload + 1, "QList<QWidget *> PDFItemDelegate::createItemWidgets() const", "list.count() != posRadioNoDownload + 1");

    /// download this file and store it locally, user will be asked for "Save As"
    radioButton = new QRadioButton(i18n("Download"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, &QRadioButton::toggled, this, &PDFItemDelegate::slotRadioDownloadToggled);
    Q_ASSERT_X(list.count() == posRadioDownload + 1, "QList<QWidget *> PDFItemDelegate::createItemWidgets() const", "list.count() != posRadioDownload + 1");

    /// paste URL into BibTeX entry, no local copy is stored
    radioButton = new QRadioButton(i18n("Use URL only"));
    bg->addButton(radioButton);
    list << radioButton;
    connect(radioButton, &QRadioButton::toggled, this, &PDFItemDelegate::slotRadioURLonlyToggled);
    Q_ASSERT_X(list.count() == posRadioURLonly + 1, "QList<QWidget *> PDFItemDelegate::createItemWidgets() const", "list.count() != posRadioURLonly + 1");

    return list;
}

/// Update the widgets
/// Clazy warns: "Missing reference on non-trivial type" for argument 'widgets',
/// but KWidgetItemDelegate defines this function this way and cannot be changed.
void PDFItemDelegate::updateItemWidgets(const QList<QWidget *> widgets, const QStyleOptionViewItem &option, const QPersistentModelIndex &index) const
{
    if (!index.isValid()) return;

    const PDFListModel *model = qobject_cast<const PDFListModel *>(index.model());
    if (model == nullptr) {
        qCDebug(LOG_KBIBTEX_GUI) << "WARNING - INVALID MODEL!";
        return;
    }

    /// determine some variables used for layout
    const int margin = option.fontMetrics.height() / 3;
    const int buttonHeight = option.fontMetrics.height() * 6 / 3;
    const int maxTextWidth = qMax(qMax(option.fontMetrics.width(i18n("Use URL only")), option.fontMetrics.width(i18n("Ignore"))), qMax(option.fontMetrics.width(i18n("Download")), option.fontMetrics.width(i18n("View"))));
    const int buttonWidth = maxTextWidth * 3 / 2;
    const int labelWidth = option.rect.width() - 3 * margin - KIconLoader::SizeMedium;
    const int labelHeight = option.fontMetrics.height();//(option.rect.height() - 4 * margin - buttonHeight) / 2;
    /// Total height = margin + labelHeight + margin + labelHeight + marin + buttonHeight + margin
    ///              = option.fontMetrics.height() * (1/3 + 1 + 1/3 + 1 + 1/3 + 6/3 + 1/3)
    ///              = option.fontMetrics.height() * 16 / 3

    /// setup label which will show the PDF file's URL
    KSqueezedTextLabel *label = qobject_cast<KSqueezedTextLabel *>(widgets[posLabelUrl]);
    if (label != nullptr) {
        const QString text = index.data(PDFListModel::URLRole).toUrl().toDisplayString();
        label->setText(text);
        label->setToolTip(text);
        label->move(margin * 2 + KIconLoader::SizeMedium, margin);
        label->resize(labelWidth, labelHeight);
    }

    /// setup label which will show the PDF's title or textual beginning
    QLabel *previewLabel = qobject_cast<QLabel *>(widgets[posLabelPreview]);
    if (previewLabel != nullptr) {
        previewLabel->setText(index.data(PDFListModel::TextualPreviewRole).toString());
        previewLabel->move(margin * 2 + KIconLoader::SizeMedium, margin * 2 + labelHeight);
        previewLabel->resize(labelWidth, labelHeight);
    }

    /// setup the view button
    QPushButton *viewButton = qobject_cast<QPushButton *>(widgets[posViewButton]);
    if (viewButton != nullptr) {
        const QSize hint = viewButton->sizeHint();
        const int h = hint.isValid() ? qMin(buttonHeight, hint.height()) : buttonHeight;
        viewButton->move(margin * 2 + KIconLoader::SizeMedium, option.rect.height() - margin - h);
        viewButton->resize(buttonWidth, h);
    }

    /// setup each of the three radio buttons
    for (int i = 0; i < 3; ++i) {
        QRadioButton *radioButton = qobject_cast<QRadioButton *>(widgets[posRadioNoDownload + i]);
        if (radioButton != nullptr) {
            const QSize hint = radioButton->sizeHint();
            const int h = hint.isValid() ? qMin(buttonHeight, hint.height()) : buttonHeight;
            radioButton->move(option.rect.width() - margin - (3 - i) * (buttonWidth + margin), option.rect.height() - margin - h);
            radioButton->resize(buttonWidth, h);
            bool ok = false;
            radioButton->setChecked(i + FindPDF::NoDownload == index.data(PDFListModel::DownloadModeRole).toInt(&ok) && ok);
        }
    }
}

QSize PDFItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const
{
    /// set a size that is suiteable
    QSize size;
    size.setWidth(option.fontMetrics.width(i18n("Download")) * 6);
    size.setHeight(qMax(option.fontMetrics.height() * 16 / 3, static_cast<int>(KIconLoader::SizeMedium))); ///< KIconLoader::SizeMedium should be 32
    return size;
}

/**
 * Method is called when the "View PDF" button of a list item is clicked.
 * Opens the associated URL or its local copy using the system's default viewer.
 */
void PDFItemDelegate::slotViewPDF()
{
    QModelIndex index = focusedIndex();

    if (index.isValid()) {
        const QString tempfileName = index.data(PDFListModel::TempFileNameRole).toString();
        const QUrl url = index.data(PDFListModel::URLRole).toUrl();
        if (!tempfileName.isEmpty()) {
            /// Guess mime type for url to open
            QUrl tempUrl(tempfileName);
            QMimeType mimeType = FileInfo::mimeTypeForUrl(tempUrl);
            const QString mimeTypeName = mimeType.name();
            /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < 0x051f00 // < 5.31.0
            KRun::runUrl(tempUrl, mimeTypeName, itemView(), false, false, url.toDisplayString());
#else // KIO_VERSION < 0x051f00 // >= 5.31.0
            KRun::runUrl(tempUrl, mimeTypeName, itemView(), KRun::RunFlags(), url.toDisplayString());
#endif // KIO_VERSION < 0x051f00
        } else if (url.isValid()) {
            /// Guess mime type for url to open
            QMimeType mimeType = FileInfo::mimeTypeForUrl(url);
            const QString mimeTypeName = mimeType.name();
            /// Ask KDE subsystem to open url in viewer matching mime type
#if KIO_VERSION < 0x051f00 // < 5.31.0
            KRun::runUrl(url, mimeTypeName, itemView(), false, false);
#else // KIO_VERSION < 0x051f00 // >= 5.31.0
            KRun::runUrl(url, mimeTypeName, itemView(), KRun::RunFlags());
#endif // KIO_VERSION < 0x051f00
        }
    }
}

/**
 * Updated the model when the user selects the radio button for ignoring a PDF file.
 */
void PDFItemDelegate::slotRadioNoDownloadToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::NoDownload, PDFListModel::DownloadModeRole);
    }
}

/**
 * Updated the model when the user selects the radio button for downloading a PDF file.
 */
void PDFItemDelegate::slotRadioDownloadToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::Download, PDFListModel::DownloadModeRole);
    }
}

/**
 * Updated the model when the user selects the radio button for keeping a PDF file's URL.
 */
void PDFItemDelegate::slotRadioURLonlyToggled(bool checked)
{
    QModelIndex index = focusedIndex();

    if (index.isValid() && checked) {
        m_parent->model()->setData(index, FindPDF::URLonly, PDFListModel::DownloadModeRole);
    }
}

PDFListModel::PDFListModel(QList<FindPDF::ResultItem> &resultList, QObject *parent)
        : QAbstractListModel(parent), m_resultList(resultList)
{
    /// nothing
}

int PDFListModel::rowCount(const QModelIndex &parent) const
{
    /// row cout depends on number of found PDF references
    int count = parent == QModelIndex() ? m_resultList.count() : 0;
    return count;
}

QVariant PDFListModel::data(const QModelIndex &index, int role) const
{
    if (index != QModelIndex() && index.parent() == QModelIndex() && index.row() < m_resultList.count()) {
        if (role == Qt::DisplayRole)
            return m_resultList[index.row()].url.toDisplayString();
        else if (role == URLRole)
            return  m_resultList[index.row()].url;
        else if (role == TextualPreviewRole)
            return  m_resultList[index.row()].textPreview;
        else if (role == Qt::ToolTipRole)
            return  QStringLiteral("<qt>") + m_resultList[index.row()].textPreview + QStringLiteral("</qt>"); ///<  'qt' tags required for word wrap
        else if (role == TempFileNameRole) {
            if (m_resultList[index.row()].tempFilename != nullptr)
                return m_resultList[index.row()].tempFilename->fileName();
            else
                return QVariant();
        } else if (role == DownloadModeRole)
            return m_resultList[index.row()].downloadMode;
        else if (role == Qt::DecorationRole) {
            /// make an educated guess on the icon, based on URL or path
            QString iconName = FileInfo::mimeTypeForUrl(m_resultList[index.row()].url).iconName();
            iconName = iconName == QStringLiteral("application-octet-stream") ? QStringLiteral("application-pdf") : iconName;
            return QIcon::fromTheme(iconName).pixmap(KIconLoader::SizeMedium, KIconLoader::SizeMedium);
        } else
            return QVariant();
    }
    return QVariant();
}

bool PDFListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index != QModelIndex() && index.row() < m_resultList.count() && role == DownloadModeRole) {
        bool ok = false;
        const FindPDF::DownloadMode downloadMode = static_cast<FindPDF::DownloadMode>(value.toInt(&ok));
        if (ok) {
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


class FindPDFUI::Private
{
private:
    FindPDFUI *p;

public:
    QListView *listViewResult;
    QLabel *labelMessage;
    QList<FindPDF::ResultItem> resultList;
    FindPDF *findpdf;

    Private(FindPDFUI *parent)
            : p(parent), findpdf(new FindPDF(parent))
    {
        setupGUI();
    }

    void setupGUI()
    {
        QGridLayout *layout = new QGridLayout(p);

        const int minWidth = p->fontMetrics().height() * 40;
        const int minHeight = p->fontMetrics().height() * 20;
        p->setMinimumSize(minWidth, minHeight);

        listViewResult = new QListView(p);
        layout->addWidget(listViewResult, 0, 0);
        listViewResult->setEnabled(false);
        listViewResult->hide();

        labelMessage = new QLabel(p);
        layout->addWidget(labelMessage, 1, 0);
        labelMessage->setMinimumSize(minWidth, minHeight);
        labelMessage->setAlignment(Qt::AlignVCenter | Qt::AlignCenter);

        static_cast<QWidget *>(p->parent())->setCursor(Qt::WaitCursor);
    }
};


FindPDFUI::FindPDFUI(Entry &entry, QWidget *parent)
        : QWidget(parent), d(new Private(this))
{
    d->labelMessage->show();
    d->labelMessage->setText(i18n("Starting to search..."));

    connect(d->findpdf, &FindPDF::finished, this, &FindPDFUI::searchFinished);
    connect(d->findpdf, &FindPDF::progress, this, &FindPDFUI::searchProgress);
    d->findpdf->search(entry);
}

FindPDFUI::~FindPDFUI()
{
    for (QList<FindPDF::ResultItem>::Iterator it = d->resultList.begin(); it != d->resultList.end();) {
        delete it->tempFilename;
        it = d->resultList.erase(it);
    }
}

void FindPDFUI::interactiveFindPDF(Entry &entry, const File &bibtexFile, QWidget *parent)
{
    QPointer<QDialog> dlg = new QDialog(parent);
    QPointer<FindPDFUI> widget = new FindPDFUI(entry, dlg);
    dlg->setWindowTitle(i18n("Find PDF"));
    QBoxLayout *layout = new QVBoxLayout(dlg);
    layout->addWidget(widget);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Abort | QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, dlg);
    layout->addWidget(buttonBox);
    dlg->setLayout(layout);

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(widget.data(), &FindPDFUI::resultAvailable, buttonBox->button(QDialogButtonBox::Ok), &QWidget::setEnabled);
    connect(widget.data(), &FindPDFUI::resultAvailable, buttonBox->button(QDialogButtonBox::Abort), &QWidget::setDisabled);
    connect(buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, dlg.data(), &QDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, dlg.data(), &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Abort), &QPushButton::clicked, widget.data(), &FindPDFUI::stopSearch);

    if (dlg->exec() == QDialog::Accepted)
        widget->apply(entry, bibtexFile);
    delete dlg;
}

void FindPDFUI::apply(Entry &entry, const File &bibtexFile)
{
    QAbstractItemModel *model = d->listViewResult->model();
    for (int i = 0; i < model->rowCount(); ++i) {
        bool ok = false;
        FindPDF::DownloadMode downloadMode = static_cast<FindPDF::DownloadMode>(model->data(model->index(i, 0), PDFListModel::DownloadModeRole).toInt(&ok));
        if (!ok) {
            qCDebug(LOG_KBIBTEX_GUI) << "Could not interprete download mode";
            downloadMode = FindPDF::NoDownload;
        }

        QUrl url = model->data(model->index(i, 0), PDFListModel::URLRole).toUrl();
        QString tempfileName = model->data(model->index(i, 0), PDFListModel::TempFileNameRole).toString();

        if (downloadMode == FindPDF::URLonly && url.isValid()) {
            bool alreadyContained = false;
            for (QMap<QString, Value>::ConstIterator it = entry.constBegin(); !alreadyContained && it != entry.constEnd(); ++it)
                // FIXME this will terribly break if URLs in an entry's URL field are separated with semicolons
                alreadyContained |= it.key().toLower().startsWith(Entry::ftUrl) && PlainTextValue::text(it.value()) == url.toDisplayString();
            if (!alreadyContained) {
                Value value;
                value.append(QSharedPointer<VerbatimText>(new VerbatimText(url.toDisplayString())));
                if (!entry.contains(Entry::ftUrl))
                    entry.insert(Entry::ftUrl, value);
                else
                    for (int i = 2; i < 256; ++i) {
                        const QString keyName = QString(QStringLiteral("%1%2")).arg(Entry::ftUrl).arg(i);
                        if (!entry.contains(keyName)) {
                            entry.insert(keyName, value);
                            break;
                        }
                    }
            }
        } else if (downloadMode == FindPDF::Download && !tempfileName.isEmpty()) {
            QUrl startUrl = bibtexFile.property(File::Url, QUrl()).toUrl();
            const QString absoluteFilename = QFileDialog::getSaveFileName(this, i18n("Save URL '%1'", url.url(QUrl::PreferLocalFile)), startUrl.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path(), QStringLiteral("application/pdf"));
            if (!absoluteFilename.isEmpty()) {
                const QString visibleFilename = UrlListEdit::askRelativeOrStaticFilename(this, absoluteFilename, startUrl);

                qCDebug(LOG_KBIBTEX_GUI) << "Saving PDF from " << url << " to file " << absoluteFilename << " known as " << visibleFilename;
                // FIXME test for overwrite
                QFile::copy(tempfileName, absoluteFilename);

                bool alreadyContained = false;
                for (QMap<QString, Value>::ConstIterator it = entry.constBegin(); !alreadyContained && it != entry.constEnd(); ++it)
                    alreadyContained |= (it.key().toLower().startsWith(Entry::ftLocalFile) || it.key().toLower().startsWith(Entry::ftUrl)) && PlainTextValue::text(it.value()) == url.toDisplayString();
                if (!alreadyContained) {
                    Value value;
                    value.append(QSharedPointer<VerbatimText>(new VerbatimText(visibleFilename)));
                    if (!entry.contains(Entry::ftLocalFile))
                        entry.insert(Entry::ftLocalFile, value);
                    else
                        for (int i = 2; i < 256; ++i) {
                            const QString keyName = QString(QStringLiteral("%1%2")).arg(Entry::ftLocalFile).arg(i);
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

void FindPDFUI::searchFinished()
{
    d->labelMessage->hide();
    d->listViewResult->show();

    d->resultList = d->findpdf->results();
    d->listViewResult->setModel(new PDFListModel(d->resultList, d->listViewResult));
    d->listViewResult->setItemDelegate(new PDFItemDelegate(d->listViewResult, d->listViewResult));
    d->listViewResult->setEnabled(true);
    d->listViewResult->reset();

    static_cast<QWidget *>(parent())->unsetCursor();
    emit resultAvailable(true);
}

void FindPDFUI::searchProgress(int visitedPages, int runningJobs, int foundDocuments)
{
    d->listViewResult->hide();
    d->labelMessage->show();
    d->labelMessage->setText(i18n("<qt>Number of visited pages: <b>%1</b><br/>Number of running downloads: <b>%2</b><br/>Number of found documents: <b>%3</b></qt>", visitedPages, runningJobs, foundDocuments));
}

void FindPDFUI::stopSearch() {
    d->findpdf->abort();
    searchFinished();
}

void FindPDFUI::abort() {
    d->findpdf->abort();
}
