/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                              and contributors                           *
 *                                                                         *
 *   Contributions to this file were made by                               *
 *   - Jurgen Spitzmuller <juergen@spitzmueller.org>                       *
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

#include "referencepreview.h"

#include <QFrame>
#include <QBuffer>
#include <QTextDocument>
#include <QLayout>
#include <QApplication>
#include <QStandardPaths>
#include <QTextStream>
#include <QTemporaryFile>
#include <QPalette>
#include <QMimeType>
#include <QDebug>
#include <QFileDialog>
#include <QPushButton>
#include <QFontDatabase>

#include <KLocalizedString>
#include <KComboBox>
#include <KRun>
#include <KIO/CopyJob>
#include <KJobWidgets>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KTextEdit>
#include <kio_version.h>

#include "fileexporterbibtex.h"
#include "fileexporterbibtex2html.h"
#include "fileexporterris.h"
#include "fileexporterxslt.h"
#include "element.h"
#include "file.h"
#include "entry.h"
#include "bibtexentries.h"
#include "fileview.h"
#include "logging_program.h"

static const struct PreviewStyles {
    QString label, style, type;
}
previewStyles[] = {
    {i18n("Source (BibTeX)"), QStringLiteral("bibtex"), QStringLiteral("exporter")},
    {i18n("Source (RIS)"), QStringLiteral("ris"), QStringLiteral("exporter")},
    {QStringLiteral("abbrv"), QStringLiteral("abbrv"), QStringLiteral("bibtex2html")},
    {QStringLiteral("acm"), QStringLiteral("acm"), QStringLiteral("bibtex2html")},
    {QStringLiteral("alpha"), QStringLiteral("alpha"), QStringLiteral("bibtex2html")},
    {QStringLiteral("apalike"), QStringLiteral("apalike"), QStringLiteral("bibtex2html")},
    {QStringLiteral("JSP"), QStringLiteral("jsp4kbibtex"), QStringLiteral("bibtex2html")},
    {QStringLiteral("ieeetr"), QStringLiteral("ieeetr"), QStringLiteral("bibtex2html")},
    {QStringLiteral("plain"), QStringLiteral("plain"), QStringLiteral("bibtex2html")},
    {QStringLiteral("siam"), QStringLiteral("siam"), QStringLiteral("bibtex2html")},
    {QStringLiteral("unsrt"), QStringLiteral("unsrt"), QStringLiteral("bibtex2html")},
    {i18n("Standard"), QStringLiteral("standard"), QStringLiteral("xml")},
    {i18n("Fancy"), QStringLiteral("fancy"), QStringLiteral("xml")},
    {i18n("Wikipedia Citation"), QStringLiteral("wikipedia-cite"), QStringLiteral("plain_xml")},
    {i18n("Abstract-only"), QStringLiteral("abstractonly"), QStringLiteral("xml")}
};
static const int previewStylesLen = sizeof(previewStyles) / sizeof(previewStyles[0]);

Q_DECLARE_METATYPE(PreviewStyles)

class ReferencePreview::ReferencePreviewPrivate
{
private:
    ReferencePreview *p;

public:
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configKeyName;

    QPushButton *buttonOpen, *buttonSaveAsHTML;
    QString htmlText;
    QUrl baseUrl;
    QTextDocument *htmlDocument;
    KTextEdit *htmlView;
    KComboBox *comboBox;
    QSharedPointer<const Element> element;
    const File *file;
    FileView *fileView;
    const QColor textColor;
    const int defaultFontSize;
    const QString htmlStart;
    const QString notAvailableMessage;

    ReferencePreviewPrivate(ReferencePreview *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))), configGroupName(QStringLiteral("Reference Preview Docklet")),
          configKeyName(QStringLiteral("Style")), file(nullptr), fileView(nullptr),
          textColor(QApplication::palette().text().color()),
          defaultFontSize(QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize()),
          htmlStart(QStringLiteral("<html>\n<head>\n<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />\n<style type=\"text/css\">\npre {\n white-space: pre-wrap;\n white-space: -moz-pre-wrap;\n white-space: -pre-wrap;\n white-space: -o-pre-wrap;\n word-wrap: break-word;\n}\n</style>\n</head>\n<body style=\"color: ") + textColor.name() + QStringLiteral("; font-size: ") + QString::number(defaultFontSize) + QStringLiteral("pt; font-family: '") + QFontDatabase::systemFont(QFontDatabase::GeneralFont).family() + QStringLiteral("'; background-color: '") + QApplication::palette().base().color().name(QColor::HexRgb) + QStringLiteral("'\">")),
          notAvailableMessage(htmlStart + QStringLiteral("<p style=\"font-style: italic;\">") + i18n("No preview available") + QStringLiteral("</p><p style=\"font-size: 90%;\">") + i18n("Reason:") + QStringLiteral(" %1</p></body></html>")) {
        QGridLayout *gridLayout = new QGridLayout(p);
        gridLayout->setMargin(0);
        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 0);

        comboBox = new KComboBox(p);
        gridLayout->addWidget(comboBox, 0, 0, 1, 3);

        QFrame *frame = new QFrame(p);
        gridLayout->addWidget(frame, 1, 0, 1, 3);
        frame->setFrameShadow(QFrame::Sunken);
        frame->setFrameShape(QFrame::StyledPanel);

        QVBoxLayout *layout = new QVBoxLayout(frame);
        layout->setMargin(0);
        htmlView = new KTextEdit(frame);
        htmlView->setReadOnly(true);
        htmlDocument = new QTextDocument(htmlView);
        htmlView->setDocument(htmlDocument);
        layout->addWidget(htmlView);

        buttonOpen = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open"), p);
        buttonOpen->setToolTip(i18n("Open reference in web browser."));
        gridLayout->addWidget(buttonOpen, 2, 1, 1, 1);

        buttonSaveAsHTML = new QPushButton(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save as HTML"), p);
        buttonSaveAsHTML->setToolTip(i18n("Save reference as HTML fragment."));
        gridLayout->addWidget(buttonSaveAsHTML, 2, 2, 1, 1);
    }

    bool saveHTML(const QUrl &url) const {
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(true);

        bool result = saveHTML(tempFile);

        if (result) {
            KIO::CopyJob *copyJob = KIO::copy(QUrl::fromLocalFile(tempFile.fileName()), url, KIO::Overwrite);
            KJobWidgets::setWindow(copyJob, p);
            result = copyJob->exec();
        }

        return result;
    }

    bool saveHTML(QTemporaryFile &tempFile) const {
        if (tempFile.open()) {
            QTextStream ts(&tempFile);
            ts.setCodec("utf-8");
            ts << QString(htmlText).replace(QRegExp(QStringLiteral("<a[^>]+href=\"kbibtex:[^>]+>([^<]+)</a>")), QStringLiteral("\\1"));
            tempFile.close();
            return true;
        }

        return false;
    }

    void loadState() {
        static bool hasBibTeX2HTML = !QStandardPaths::findExecutable(QStringLiteral("bibtex2html")).isEmpty();

        int styleIndex = 0;
        KConfigGroup configGroup(config, configGroupName);
        const QString previousStyle = configGroup.readEntry(configKeyName, QString());

        comboBox->clear();
        for (int i = 0, c = 0; i < previewStylesLen; ++i) {
            if (!hasBibTeX2HTML && previewStyles[i].type.contains(QStringLiteral("bibtex2html"))) continue;
            comboBox->addItem(previewStyles[i].label, QVariant::fromValue(previewStyles[i]));
            if (previousStyle == previewStyles[i].style)
                styleIndex = c;
            ++c;
        }
        comboBox->setCurrentIndex(styleIndex);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyName, comboBox->itemData(comboBox->currentIndex()).value<PreviewStyles>().style);
        config->sync();
    }
};

ReferencePreview::ReferencePreview(QWidget *parent)
        : QWidget(parent), d(new ReferencePreviewPrivate(this))
{
    d->loadState();

    connect(d->buttonOpen, &QPushButton::clicked, this, &ReferencePreview::openAsHTML);
    connect(d->buttonSaveAsHTML, &QPushButton::clicked, this, &ReferencePreview::saveAsHTML);
    connect(d->comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ReferencePreview::renderHTML);

    setEnabled(false);
}

ReferencePreview::~ReferencePreview()
{
    delete d;
}

void ReferencePreview::setHtml(const QString &html, bool buttonsEnabled)
{
    d->htmlText = QString(html).remove(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
    d->htmlDocument->setHtml(d->htmlText);
    d->buttonOpen->setEnabled(buttonsEnabled);
    d->buttonSaveAsHTML->setEnabled(buttonsEnabled);
}

void ReferencePreview::setEnabled(bool enabled)
{
    if (enabled)
        setHtml(d->htmlText, true);
    else
        setHtml(d->notAvailableMessage.arg(i18n("Preview disabled")), false);
    d->htmlView->setEnabled(enabled);
    d->comboBox->setEnabled(enabled);
}

void ReferencePreview::setElement(QSharedPointer<Element> element, const File *file)
{
    d->element = element;
    d->file = file;
    renderHTML();
}

void ReferencePreview::renderHTML()
{
    enum { ignore, /// do not include crossref'ed entry's values (one entry)
           /// NOT USED: add, /// feed both the current entry as well as the crossref'ed entry into the exporter (two entries)
           merge /// merge the crossref'ed entry's values into the current entry (one entry)
         } crossRefHandling = ignore;

    if (d->element.isNull()) {
        setHtml(d->notAvailableMessage.arg(i18n("No element selected")), false);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    FileExporter *exporter = nullptr;

    const PreviewStyles previewStyle = d->comboBox->itemData(d->comboBox->currentIndex()).value<PreviewStyles>();

    if (previewStyle.type == QStringLiteral("exporter")) {
        if (previewStyle.style == QStringLiteral("bibtex")) {
            FileExporterBibTeX *exporterBibTeX = new FileExporterBibTeX(this);
            exporterBibTeX->setEncoding(QStringLiteral("utf-8"));
            exporter = exporterBibTeX;
        } else if (previewStyle.style == QStringLiteral("ris"))
            exporter = new FileExporterRIS(this);
        else
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Don't know how to handle output style " << previewStyle.style << " for type " << previewStyle.type;
    } else if (previewStyle.type == QStringLiteral("bibtex2html")) {
        crossRefHandling = merge;
        FileExporterBibTeX2HTML *exporterHTML = new FileExporterBibTeX2HTML(this);
        exporterHTML->setLaTeXBibliographyStyle(previewStyle.style);
        exporter = exporterHTML;
    } else if (previewStyle.type == QStringLiteral("xml") || previewStyle.type.endsWith(QStringLiteral("_xml"))) {
        crossRefHandling = merge;
        const QString filename = previewStyle.style + QStringLiteral(".xsl");
        exporter = new FileExporterXSLT(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kbibtex/") + filename), this);
    } else
        qCWarning(LOG_KBIBTEX_PROGRAM) << "Don't know how to handle output type " << previewStyle.type;

    if (exporter != nullptr) {
        QBuffer buffer(this);
        buffer.open(QBuffer::WriteOnly);

        bool exporterResult = false;
        QStringList errorLog;
        QSharedPointer<const Entry> entry = d->element.dynamicCast<const Entry>();
        /** NOT USED
        if (crossRefHandling == add && !entry.isNull()) {
            QString crossRef = PlainTextValue::text(entry->value(QStringLiteral("crossref")));
            QSharedPointer<const Entry> crossRefEntry = d->file == NULL ? QSharedPointer<const Entry>() : d->file->containsKey(crossRef) .dynamicCast<const Entry>();
            if (!crossRefEntry.isNull()) {
                File file;
                file.append(QSharedPointer<Entry>(new Entry(*entry)));
                file.append(QSharedPointer<Entry>(new Entry(*crossRefEntry)));
                exporterResult = exporter->save(&buffer, &file, &errorLog);
            } else
                exporterResult = exporter->save(&buffer, d->element, d->file, &errorLog);
        } else */
        if (crossRefHandling == merge && !entry.isNull()) {
            QSharedPointer<Entry> merged = QSharedPointer<Entry>(Entry::resolveCrossref(*entry, d->file, BibTeXEntries::self()->xmappings(entry->type())));
            exporterResult = exporter->save(&buffer, merged, d->file, &errorLog);
        } else
            exporterResult = exporter->save(&buffer, d->element, d->file, &errorLog);
        buffer.close();
        delete exporter;

        buffer.open(QBuffer::ReadOnly);
        QString text = QString::fromUtf8(buffer.readAll().constData());
        buffer.close();

        bool buttonsEnabled = true;

        if (!exporterResult || text.isEmpty()) {
            /// something went wrong, no output ...
            text = d->notAvailableMessage.arg(i18n("No output generated"));
            buttonsEnabled = false;
            qCDebug(LOG_KBIBTEX_PROGRAM) << errorLog.join(QStringLiteral("\n"));
        } else {
            /// beautify text
            text.replace(QStringLiteral("``"), QStringLiteral("&ldquo;"));
            text.replace(QStringLiteral("''"), QStringLiteral("&rdquo;"));
            static const QRegExp openingSingleQuotationRegExp(QStringLiteral("(^|[> ,.;:!?])`(\\S)"));
            static const QRegExp closingSingleQuotationRegExp(QStringLiteral("(\\S)'([ ,.;:!?<]|$)"));
            text.replace(openingSingleQuotationRegExp, QStringLiteral("\\1&lsquo;\\2"));
            text.replace(closingSingleQuotationRegExp, QStringLiteral("\\1&rsquo;\\2"));

            if (previewStyle.style == QStringLiteral("wikipedia-cite"))
                text.remove(QStringLiteral("\n"));

            if (text.contains(QStringLiteral("{{cite FIXME"))) {
                /// Wikipedia {{cite ...}} command had problems (e.g. unknown entry type)
                text = d->notAvailableMessage.arg(i18n("This type of element is not supported by Wikipedia's <tt>{{cite}}</tt> command."));
            } else if (previewStyle.type == QStringLiteral("exporter") || previewStyle.type.startsWith(QStringLiteral("plain_"))) {
                /// source
                text.prepend(QStringLiteral("';\">"));
                text.prepend(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
                text.prepend(QStringLiteral("<pre style=\"font-family: '"));
                text.prepend(d->htmlStart);
                text.append(QStringLiteral("</pre></body></html>"));
            } else if (previewStyle.type == QStringLiteral("bibtex2html")) {
                /// bibtex2html

                /// remove "generated by" line from HTML code if BibTeX2HTML was used
                text.remove(QRegExp(QStringLiteral("<hr><p><em>.*</p>")));
                text.remove(QRegExp(QStringLiteral("<[/]?(font)[^>]*>")));
                QRegExp reTable(QStringLiteral("^.*<td.*</td.*<td>"));
                reTable.setMinimal(true);
                text.remove(reTable);
                text.remove(QRegExp(QStringLiteral("</td>.*$")));
                QRegExp reAnchor(QStringLiteral("\\[ <a.*</a> \\]"));
                reAnchor.setMinimal(true);
                text.remove(reAnchor);

                /// replace ASCII art with Unicode characters
                text.replace(QStringLiteral("---"), QString(QChar(0x2014)));
                text.replace(QStringLiteral("--"), QString(QChar(0x2013)));

                text.prepend(d->htmlStart);
                text.append("</body></html>");
            } else if (previewStyle.type == QStringLiteral("xml")) {
                /// XML/XSLT
                text.prepend(d->htmlStart);
                text.append("</body></html>");
            }

            /// adopt current color scheme
            text.replace(QStringLiteral("color: black;"), QString(QStringLiteral("color: %1;")).arg(d->textColor.name()));
        }

        setHtml(text, buttonsEnabled);

        d->saveState();
    } else {
        /// something went wrong, no exporter ...
        setHtml(d->notAvailableMessage.arg(i18n("No output generated")), false);
    }

    QApplication::restoreOverrideCursor();
}

void ReferencePreview::openAsHTML()
{
    QTemporaryFile file(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + QStringLiteral("referencePreview-openAsHTML-XXXXXX.html"));
    file.setAutoRemove(false); /// let file stay alive for browser
    d->saveHTML(file);

    /// Ask KDE subsystem to open url in viewer matching mime type
    QUrl url(file.fileName());
#if KIO_VERSION < 0x051f00 // < 5.31.0
    KRun::runUrl(url, QStringLiteral("text/html"), this, false, false);
#else // KIO_VERSION < 0x051f00 // >= 5.31.0
    KRun::runUrl(url, QStringLiteral("text/html"), this, KRun::RunFlags());
#endif // KIO_VERSION < 0x051f00
}

void ReferencePreview::saveAsHTML()
{
    QUrl url = QFileDialog::getSaveFileUrl(this, i18n("Save as HTML"), QUrl(), QStringLiteral("text/html"));
    if (url.isValid())
        d->saveHTML(url);
}

void ReferencePreview::linkClicked(const QUrl &url)
{
    QString text = url.toDisplayString();
    if (text.startsWith(QStringLiteral("kbibtex:filter:"))) {
        text = text.mid(15);
        if (d->fileView != nullptr) {
            int p = text.indexOf(QStringLiteral("="));
            SortFilterFileModel::FilterQuery fq;
            fq.terms << text.mid(p + 1);
            fq.combination = SortFilterFileModel::EveryTerm;
            fq.field = text.left(p);
            fq.searchPDFfiles = false;
            d->fileView->setFilterBarFilter(fq);
        }
    }
}

void ReferencePreview::setFileView(FileView *fileView)
{
    d->fileView = fileView;
}
