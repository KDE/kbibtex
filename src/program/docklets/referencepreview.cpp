/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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
#include <QTextStream>
#include <QTemporaryFile>
#include <QPalette>
#include <QMimeType>
#include <QFileDialog>
#include <QPushButton>
#include <QFontDatabase>
#include <QComboBox>
#include <QPointer>

#include <kio_version.h>
#include <KLocalizedString>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
#include <KIO/OpenUrlJob>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
#include <KIO/JobUiDelegateFactory>
#else // < 5.98.0
#include <KIO/JobUiDelegate>
#endif // QT_VERSION_CHECK(5, 98, 0)
#else // < 5.71.0
#include <KRun>
#endif // KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
#include <KIO/CopyJob>
#include <KJobWidgets>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KTextEdit>

#include <Element>
#include <Entry>
#include <File>
#include <FileExporterBibTeX>
#include <FileExporterBibTeX2HTML>
#include <FileExporterRIS>
#include <FileExporterXML>
#include <file/FileView>
#include "logging_program.h"

typedef struct {
    int id;
    QString label;
    QHash<QString, QVariant> attributes;
} PreviewStyle;

Q_DECLARE_METATYPE(PreviewStyle)

class ReferencePreview::Private
{
private:
    ReferencePreview *p;

    QVector<PreviewStyle> previewStyles() const {
        static QVector<PreviewStyle> listOfStyles;
        if (listOfStyles.isEmpty()) {
            listOfStyles = {
                {1000, i18n("Source (BibTeX)"), {{QStringLiteral("exporter"), QStringLiteral("bibtex")}, {QStringLiteral("fixed-font"), true}}},
                {1010, i18n("Source (RIS)"), {{QStringLiteral("exporter"), QStringLiteral("ris")}, {QStringLiteral("fixed-font"), true}}},
                {1210, i18n("Standard"), {{QStringLiteral("exporter"), QStringLiteral("xml")}, {QStringLiteral("style"), QVariant::fromValue(FileExporterXML::OutputStyle::HTML_Standard)}, {QStringLiteral("html"), true}}},
                {1220, i18n("Wikipedia Citation"), {{QStringLiteral("exporter"), QStringLiteral("xml")}, {QStringLiteral("style"), QVariant::fromValue(FileExporterXML::OutputStyle::Plain_WikipediaCite)}, {QStringLiteral("fixed-font"), true}}},
                {1230, i18n("Abstract-only"), {{QStringLiteral("exporter"), QStringLiteral("xml")}, {QStringLiteral("style"), QVariant::fromValue(FileExporterXML::OutputStyle::HTML_AbstractOnly)}, {QStringLiteral("html"), true}}}
            };
            if (!QStandardPaths::findExecutable(QStringLiteral("bibtex2html")).isEmpty()) {
                // If 'bibtex2html' binary is available ...
                int id = 2000;
                // Query from FileExporterBibTeX2HTML which BibTeX styles are available
                for (const QString &bibtex2htmlStyle : FileExporterBibTeX2HTML::availableLaTeXBibliographyStyles()) {
                    const QString label{QString(QStringLiteral("%2 (bibtex2html)")).arg(bibtex2htmlStyle)};
                    listOfStyles.append({id++, label, {{QStringLiteral("exporter"), QStringLiteral("bibtex2html")}, {QStringLiteral("html"), true}, {QStringLiteral("externalprogram"), true}, {QStringLiteral("bibtexstyle"), bibtex2htmlStyle}}});
                }
            }
        }
        return listOfStyles;
    }

public:
    KSharedConfigPtr config;
    static const QString configGroupName;
    static const QString configKeyName;
    static const QString errorMessageInnerTemplate;
    const QString htmlStart, errorMessageOuterTemplate;

    QPushButton *buttonOpen, *buttonSaveAsHTML;
    QString htmlText;
    QUrl baseUrl;
    QTextDocument *htmlDocument;
    KTextEdit *htmlView;
    QComboBox *comboBox;
    QSharedPointer<const Element> element;
    const File *file;
    FileView *fileView;

    Private(ReferencePreview *parent)
            : p(parent), config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc"))),
          htmlStart(QString(QStringLiteral("<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n<style type=\"text/css\">\npre {\n white-space: pre-wrap;\n white-space: -moz-pre-wrap;\n white-space: -pre-wrap;\n white-space: -o-pre-wrap;\n word-wrap: break-word;\n}\n</style>\n</head>\n<body style=\"color: %1; background-color: '%2'; font-family: '%3'; font-size: %4pt\">")).arg(QApplication::palette().text().color().name(QColor::HexRgb), QApplication::palette().base().color().name(QColor::HexRgb), QFontDatabase::systemFont(QFontDatabase::GeneralFont).family(), QString::number(QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize()))),
          errorMessageOuterTemplate(htmlStart + errorMessageInnerTemplate + QStringLiteral("</body></html>")),
          file(nullptr), fileView(nullptr)
    {
        QGridLayout *gridLayout = new QGridLayout(p);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setColumnStretch(0, 1);
        gridLayout->setColumnStretch(1, 0);
        gridLayout->setColumnStretch(2, 0);

        comboBox = new QComboBox(p);
        gridLayout->addWidget(comboBox, 0, 0, 1, 3);

        QFrame *frame = new QFrame(p);
        gridLayout->addWidget(frame, 1, 0, 1, 3);
        frame->setFrameShadow(QFrame::Sunken);
        frame->setFrameShape(QFrame::StyledPanel);

        QVBoxLayout *layout = new QVBoxLayout(frame);
        layout->setContentsMargins(0, 0, 0, 0);
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
            // https://forum.qt.io/topic/135724/qt-6-replacement-for-qtextcodec
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
            ts.setCodec("utf-8");
#else
            ts.setEncoding(QStringConverter::Utf8);
#endif
            static const QRegularExpression kbibtexHrefRegExp(QStringLiteral("<a[^>]+href=\"kbibtex:[^>]+>(.+?)</a>"));
            QString modifiedHtmlText = htmlText;
            modifiedHtmlText = modifiedHtmlText.replace(kbibtexHrefRegExp, QStringLiteral("\\1"));
            ts << modifiedHtmlText;
            tempFile.close();
            return true;
        }

        return false;
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        const int previousStyleId = configGroup.readEntry(configKeyName, -1);

        comboBox->clear();

        int styleIndex = 0, c = 0;
        for (const PreviewStyle &previewStyle : previewStyles()) {
            comboBox->addItem(previewStyle.label, QVariant::fromValue(previewStyle));
            if (previousStyleId == previewStyle.id)
                styleIndex = c;
            ++c;
        }
        comboBox->setCurrentIndex(styleIndex);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyName, comboBox->itemData(comboBox->currentIndex()).value<PreviewStyle>().id);
        config->sync();
    }

    void setHtml(const QString &html, bool buttonsEnabled)
    {
        htmlText = QString(html).remove(QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"));
        htmlDocument->setHtml(htmlText);
        buttonOpen->setEnabled(buttonsEnabled);
        buttonSaveAsHTML->setEnabled(buttonsEnabled);
    }
};

const QString ReferencePreview::Private::configGroupName {QStringLiteral("Reference Preview Docklet")};
const QString ReferencePreview::Private::configKeyName {QStringLiteral("Style")};
const QString ReferencePreview::Private::errorMessageInnerTemplate {QString(QStringLiteral("<p style=\"font-style: italic;\">%1</p><p style=\"font-size: 90%;\">%2 %3</p>")).arg(i18n("No preview available"), i18n("Reason:"))};

ReferencePreview::ReferencePreview(QWidget *parent)
        : QWidget(parent), d(new Private(this))
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

void ReferencePreview::setEnabled(bool enabled)
{
    if (enabled)
        d->setHtml(d->htmlText, true);
    else
        d->setHtml(d->errorMessageOuterTemplate.arg(i18nc("Message in case reference preview widget is disabled", "Preview disabled")), false);
    d->htmlView->setEnabled(enabled);
    d->comboBox->setEnabled(enabled);
}

void ReferencePreview::setElement(QSharedPointer<const Element> element, const File *file)
{
    d->element = element;
    d->file = file;
    renderHTML();
}

void ReferencePreview::renderHTML()
{
    if (d->element.isNull()) {
        d->setHtml(d->errorMessageOuterTemplate.arg(i18nc("Message in case no bibliographic element is selected for preview", "No element selected")), false);
        return;
    }

    const bool elementIsEntry {!d->element.dynamicCast<const Entry>().isNull()};
    const PreviewStyle previewStyle = d->comboBox->itemData(d->comboBox->currentIndex()).value<PreviewStyle>();

    const QString exporterName = previewStyle.attributes.value(QStringLiteral("exporter"), QString()).toString();
    QScopedPointer<FileExporter> exporter([this, &previewStyle, &exporterName]() {
        if (exporterName == QStringLiteral("bibtex")) {
            FileExporterBibTeX *exporterBibTeX = new FileExporterBibTeX(this);
            exporterBibTeX->setEncoding(QStringLiteral("utf-8"));
            return qobject_cast<FileExporter *>(exporterBibTeX);
        } else if (exporterName == QStringLiteral("ris"))
            return qobject_cast<FileExporter * >(new FileExporterRIS(this));
        else if (exporterName == QStringLiteral("bibtex2html")) {
            FileExporterBibTeX2HTML *exporterBibTeX2HTML = new FileExporterBibTeX2HTML(this);
            exporterBibTeX2HTML->setLaTeXBibliographyStyle(previewStyle.attributes.value(QStringLiteral("bibtexstyle"), QString()).toString());
            return qobject_cast<FileExporter *>(exporterBibTeX2HTML);
        } else if (exporterName == QStringLiteral("xml")) {
            FileExporterXML *exporterXML = new FileExporterXML(this);
            exporterXML->setOutputStyle(previewStyle.attributes.value(QStringLiteral("style"), QVariant::fromValue(FileExporterXML::OutputStyle::XML_KBibTeX)).value<FileExporterXML::OutputStyle>());
            return qobject_cast<FileExporter *>(exporterXML);
        } else if (!exporterName.isEmpty())
            qCWarning(LOG_KBIBTEX_PROGRAM) << "Don't know how to handle exporter " << exporterName << " for preview style " << previewStyle.label << "(" << previewStyle.id << ")";

        return static_cast<FileExporter *>(nullptr);
    }());

    QString text;
    bool exporterResult = false;
    bool textIsErrorMessage = false;
    if (!exporter.isNull()) {
        const bool externalProgramInvocation {previewStyle.attributes.value(QStringLiteral("externalprogram"), false).toBool()};
        if (externalProgramInvocation) {
            // Exporter may invoke external programs which take time, so set the busy cursor
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        }
        QBuffer buffer(this);
        buffer.open(QBuffer::WriteOnly);
        exporterResult = exporter->save(&buffer, d->element, d->file);
        buffer.close();
        if (externalProgramInvocation)
            QApplication::restoreOverrideCursor();

        buffer.open(QBuffer::ReadOnly);
        text = QString::fromUtf8(buffer.readAll().constData()).trimmed();
        buffer.close();
    } else {
        text = d->errorMessageInnerTemplate.arg(i18nc("No exporter clould be located to generate output", "No output generated"));
        textIsErrorMessage = true;
    }

    // Remove comments
    int p = 0;
    while ((p = text.indexOf(QStringLiteral("<!--"), p)) >= 0) {
        const int p2 = text.indexOf(QStringLiteral("-->"), p + 3);
        if (p2 > p) {
            text = text.left(p).trimmed() + text.mid(p2 + 3).trimmed();
        } else
            break;
    }

    if (exporterName == QStringLiteral("bibtex")) {
        // Remove special comments from BibTeX code
        int xsomethingpos = 0;
        while ((xsomethingpos = text.indexOf(QStringLiteral("@comment{x-"), xsomethingpos)) >= 0) {
            int endofxsomethingpos = text.indexOf(QStringLiteral("}"), xsomethingpos + 11);
            if (endofxsomethingpos > xsomethingpos) {
                // Trim empty lines around match
                while (xsomethingpos > 0 && text[xsomethingpos - 1] == QLatin1Char('\n')) --xsomethingpos;
                while (endofxsomethingpos + 1 < text.length() && text[endofxsomethingpos + 1] == QLatin1Char('\n')) ++endofxsomethingpos;
                // Clip comment out of text
                text = text.left(xsomethingpos) + text.mid(endofxsomethingpos + 1);
            }
        }
    } else if (exporterName == QStringLiteral("bibtex2html")) {
        // Remove bibtex2html's output after the horizontal line
        const int hrPos = text.indexOf(QStringLiteral("<hr"));
        if (hrPos >= 0)
            text = text.left(hrPos);

        // Remove various HTML artifacts
        static const QSet<const QRegularExpression> toBeRemovedSet{QRegularExpression(QStringLiteral("<[/]?(font)[^>]*>")), QRegularExpression(QStringLiteral("^.*?<td.*?</td.*?<td>")), QRegularExpression(QStringLiteral("</td>.*$")), QRegularExpression(QStringLiteral("\\[ <a.*?</a> \\]"))};
        for (const auto &toBeRemoved : toBeRemovedSet)
            text.remove(toBeRemoved);
    }

    if ((!exporterResult || text.isEmpty()) && !elementIsEntry) {
        // Some exporters do not handle things like macros or preamble.
        // Assume that this is the case here and provide an approriate error message for that
        text = d->errorMessageInnerTemplate.arg(i18n("Cannot show a preview for this type of element"));
        textIsErrorMessage = true;
    }

    if (!textIsErrorMessage && previewStyle.attributes.value(QStringLiteral("fixed-font"), false).toBool()) {
        // Preview text gets formatted as verbatim text in monospaced font
        text.prepend(QStringLiteral("';\">"));
        text.prepend(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
        text.prepend(QStringLiteral("<pre style=\"font-family: '"));
        text.append(QStringLiteral("</pre>"));
    } else if (!textIsErrorMessage && previewStyle.attributes.value(QStringLiteral("html"), false).toBool()) {
        // Remove existing header up to and including <body>
        int p = text.indexOf(QStringLiteral("<body"));
        if (p >= 0) {
            p = text.indexOf(QStringLiteral(">"), p + 4);
            if (p >= 0)
                text = text.mid(p + 1).trimmed();
        }
        // Remove existing footer starting with </body>
        p = text.indexOf(QStringLiteral("</body"));
        if (p >= 0)
            text = text.left(p).trimmed();
    }

    if (previewStyle.attributes.value(QStringLiteral("style"), QVariant::fromValue(FileExporterXML::OutputStyle::XML_KBibTeX)).value<FileExporterXML::OutputStyle>() == FileExporterXML::OutputStyle::Plain_WikipediaCite)
        text.remove(QStringLiteral("\n")).replace(QStringLiteral("| "), QStringLiteral("|"));
    else if (previewStyle.attributes.value(QStringLiteral("style"), QVariant::fromValue(FileExporterXML::OutputStyle::XML_KBibTeX)).value<FileExporterXML::OutputStyle>() == FileExporterXML::OutputStyle::HTML_AbstractOnly) {
        if (text.isEmpty()) {
            text = d->errorMessageInnerTemplate.arg(i18nc("Cannot show a reference preview as entry does not contain an abstract", "No abstract"));
            textIsErrorMessage = true;
        }
    }

    // Beautify text
    text.replace(QStringLiteral("``"), QStringLiteral("&ldquo;"));
    text.replace(QStringLiteral("''"), QStringLiteral("&rdquo;"));
    static const QRegularExpression openingSingleQuotationRegExp(QStringLiteral("(^|[> ,.;:!?])`(\\S)"));
    static const QRegularExpression closingSingleQuotationRegExp(QStringLiteral("(\\S)'([ ,.;:!?<]|$)"));
    text.replace(openingSingleQuotationRegExp, QStringLiteral("\\1&lsquo;\\2"));
    text.replace(closingSingleQuotationRegExp, QStringLiteral("\\1&rsquo;\\2"));

    // Replace ASCII art with Unicode characters
    text.replace(QStringLiteral("---"), QString(QChar(0x2014)));
    text.replace(QStringLiteral("--"), QString(QChar(0x2013)));

    // Add common HTML start and end
    text.prepend(d->htmlStart);
    text.append(QStringLiteral("</body></html>"));

    d->setHtml(text, !textIsErrorMessage);
    d->saveState();
}

void ReferencePreview::openAsHTML()
{
    QTemporaryFile file(QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QDir::separator() + QStringLiteral("referencePreview-openAsHTML-XXXXXX.html"));
    file.setAutoRemove(false); /// let file stay alive for browser
    d->saveHTML(file);

    /// Ask KDE subsystem to open url in viewer matching mime type
    const QUrl url{QUrl::fromLocalFile(file.fileName())};
#if KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
    KRun::runUrl(url, QStringLiteral("text/html"), this, KRun::RunFlags());
#else // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0) // >= 5.71.0
    KIO::OpenUrlJob *job = new KIO::OpenUrlJob(url, QStringLiteral("text/html"));
#if KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // < 5.98.0
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
#else // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0) // >= 5.98.0
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 98, 0)
    job->start();
#endif // KIO_VERSION < QT_VERSION_CHECK(5, 71, 0)
}

void ReferencePreview::saveAsHTML()
{
    QPointer<QFileDialog> dlg = new QFileDialog(this, i18n("Save as HTML"));
    dlg->setMimeTypeFilters({QStringLiteral("text/html")});
    const int result = dlg->exec();
    QUrl url;
    if (result == QDialog::Accepted && !dlg->selectedUrls().isEmpty() && (url = dlg->selectedUrls().first()).isValid())
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
            fq.combination = SortFilterFileModel::FilterCombination::EveryTerm;
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
