/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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

#include <QFrame>
#include <QBuffer>
#include <QWebView>
#include <QLayout>
#include <QApplication>
#include <QTextStream>
#include <QDesktopServices>

#include <KTemporaryFile>
#include <KLocale>
#include <KComboBox>
#include <KStandardDirs>
#include <KPushButton>
#include <KFileDialog>
#include <KDebug>
#include <kio/netaccess.h>

#include <fileexporterbibtex.h>
#include <fileexporterbibtex2html.h>
#include <fileexporterxslt.h>
#include <element.h>
#include <file.h>
#include <entry.h>

#include "referencepreview.h"

const QString notAvailableMessage = "<html><body style=\"font-family: '" + KGlobalSettings::generalFont().family() + "';\"><p style=\"font-size: " + QString::number(KGlobalSettings::generalFont().pointSize()) + "pt; font-style: italic; color: #333;\">" + i18n("No preview available") + "</p><p style=\"font-size: " + QString::number(KGlobalSettings::generalFont().pointSize() * .9) + "pt; color: #666;\">" + i18n("Reason:") + " %1</p></body></html>"; //FIXME: Font size seems to be too small
class ReferencePreview::ReferencePreviewPrivate
{
private:
    ReferencePreview *p;

public:
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configKeyName;

    KPushButton *buttonOpen, *buttonSaveAsHTML;
    QString htmlText;
    QUrl baseUrl;
    QWebView *webView;
    KComboBox *comboBox;
    const Element* element;
    const File *file;

    ReferencePreviewPrivate(ReferencePreview *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("Reference Preview Docklet")),
            configKeyName(QLatin1String("Style")), element(NULL) {
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
        webView = new QWebView(frame);
        layout->addWidget(webView);

        buttonOpen = new KPushButton(KIcon("document-open"), i18n("Open"), p);
        buttonOpen->setToolTip(i18n("Open reference in web browser."));
        gridLayout->addWidget(buttonOpen, 2, 1, 1, 1);

        buttonSaveAsHTML = new KPushButton(KIcon("document-save"), i18n("Save as HTML"), p);
        buttonSaveAsHTML->setToolTip(i18n("Save reference as HTML fragment."));
        gridLayout->addWidget(buttonSaveAsHTML, 2, 2, 1, 1);

        comboBox->addItem(i18n("Source"));
        comboBox->addItem(i18n("abbrv (bibtex2html)"));
        comboBox->addItem(i18n("acm (bibtex2html)"));
        comboBox->addItem(i18n("alpha (bibtex2html)"));
        comboBox->addItem(i18n("apalike (bibtex2html)"));
        comboBox->addItem(i18n("ieeetr (bibtex2html)"));
        comboBox->addItem(i18n("plain (bibtex2html)"));
        comboBox->addItem(i18n("siam (bibtex2html)"));
        comboBox->addItem(i18n("unsrt (bibtex2html)"));
        comboBox->addItem(i18n("standard (XML/XSLT)"));
        comboBox->addItem(i18n("fancy (XML/XSLT)"));
    }

    bool saveHTML(const KUrl& url) {
        KTemporaryFile file;
        file.setAutoRemove(true);

        bool result = saveHTML(file);

        if (result) {
            KIO::NetAccess::del(url, p); /// ignore error if file does not exist
            result = KIO::NetAccess::file_copy(KUrl(file.fileName()), url, p);
        }

        return result;
    }

    bool saveHTML(KTemporaryFile &file) {
        if (file.open()) {
            QTextStream ts(&file);
            ts << htmlText;
            file.close();
            return true;
        }

        return false;
    }

    void loadState() {
        KConfigGroup configGroup(config, configGroupName);
        int styleIndex = configGroup.readEntry(configKeyName, 0);
        comboBox->setCurrentIndex(styleIndex);
    }

    void saveState() {
        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyName, comboBox->currentIndex());
        config->sync();
    }
};

ReferencePreview::ReferencePreview(QWidget *parent)
        : QWidget(parent), d(new ReferencePreviewPrivate(this))
{
    setEnabled(false);

    d->loadState();

    connect(d->buttonOpen, SIGNAL(clicked()), this, SLOT(openAsHTML()));
    connect(d->buttonSaveAsHTML, SIGNAL(clicked()), this, SLOT(saveAsHTML()));
    connect(d->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(renderHTML()));
}

void ReferencePreview::setHtml(const QString & html, const QUrl & baseUrl)
{
    d->htmlText = html;
    d->baseUrl = baseUrl;
    d->webView->setHtml(html, baseUrl);
    d->buttonOpen->setEnabled(true);
    d->buttonSaveAsHTML->setEnabled(true);
}

void ReferencePreview::setEnabled(bool enabled)
{
    if (enabled)
        setHtml(d->htmlText, d->baseUrl);
    else {
        d->webView->setHtml(notAvailableMessage.arg(i18n("Preview disabled")), d->baseUrl);
        d->buttonOpen->setEnabled(false);
        d->buttonSaveAsHTML->setEnabled(false);
    }
    d->webView->setEnabled(enabled);
    d->comboBox->setEnabled(enabled);
}

void ReferencePreview::setElement(Element* element, const File *file)
{
    d->element = element;
    d->file = file;
    renderHTML();
}

void ReferencePreview::renderHTML()
{
    enum { ignore, /// do not include crossref'ed entry's values (one entry)
           add, /// feed both the current entry as well as the crossref'ed entry into the exporter (two entries)
           merge /// merge the crossref'ed entry's values into the current entry (one entry)
         } crossRefHandling = ignore;

    if (d->element == NULL) {
        d->webView->setHtml(notAvailableMessage.arg(i18n("No element selected")), d->baseUrl);
        d->buttonOpen->setEnabled(false);
        d->buttonSaveAsHTML->setEnabled(false);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QStringList errorLog;
    FileExporter *exporter = NULL;

    if (d->comboBox->currentIndex() == 0)
        exporter = new FileExporterBibTeX();
    else if (d->comboBox->currentIndex() < 9) {
        crossRefHandling = merge;
        FileExporterBibTeX2HTML *exporterHTML = new FileExporterBibTeX2HTML();
        switch (d->comboBox->currentIndex()) {
        case 1: /// BibTeX2HTML (abbrv)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("abbrv"));
            break;
        case 2: /// BibTeX2HTML (acm)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("acm"));
            break;
        case 3: /// BibTeX2HTML (alpha)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("alpha"));
            break;
        case 4: /// BibTeX2HTML (apalike)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("apalike"));
            break;
        case 5: /// BibTeX2HTML (ieeetr)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("ieeetr"));
            break;
        case 6: /// BibTeX2HTML (plain)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("plain"));
            break;
        case 7: /// BibTeX2HTML (siam)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("siam"));
            break;
        case 8: /// BibTeX2HTML (unsrt)
            exporterHTML->setLaTeXBibliographyStyle(QLatin1String("unsrt"));
            break;
        }
        exporter = exporterHTML;
    } else {
        crossRefHandling = merge;
        FileExporterXSLT *exporterXSLT = new FileExporterXSLT();
        switch (d->comboBox->currentIndex()) {
        case 9: /// XML/XSLT (standard)
            exporterXSLT->setXSLTFilename(KStandardDirs::locate("appdata", "standard.xsl"));
            break;
        case 10: /// XML/XSLT (fancy)
            exporterXSLT->setXSLTFilename(KStandardDirs::locate("appdata", "fancy.xsl"));
            break;
        }
        exporter = exporterXSLT;
    }

    QBuffer buffer(this);
    buffer.open(QBuffer::WriteOnly);

    const Entry *entry = dynamic_cast<const Entry*>(d->element);
    if (crossRefHandling == add && entry != NULL) {
        QString crossRef = PlainTextValue::text(entry->value(QLatin1String("crossref")), d->file);
        const Entry *crossRefEntry = dynamic_cast<const Entry*>((d->file != NULL) ? d->file->containsKey(crossRef) : NULL);
        if (crossRefEntry != NULL) {
            File file;
            file.append(new Entry(*entry));
            file.append(new Entry(*crossRefEntry));
            exporter->save(&buffer, &file, &errorLog);
        } else
            exporter->save(&buffer, d->element, &errorLog);
    } else if (crossRefHandling == merge && entry != NULL) {
        Entry *merged = Entry::resolveCrossref(*entry, d->file);
        exporter->save(&buffer, merged, &errorLog);
        delete merged;
    } else
        exporter->save(&buffer, d->element, &errorLog);
    buffer.close();
    delete exporter;

    buffer.open(QBuffer::ReadOnly);
    QTextStream ts(&buffer);
    QString text = ts.readAll();
    buffer.close();

    if (text.isEmpty()) {
        /// something went wrong, no output ...
        text = notAvailableMessage.arg(i18n("No HTML output generated"));
        kDebug() << errorLog.join("\n");
    }

    if (d->comboBox->currentIndex() == 0) {
        /// source
        text.prepend("<html><body><pre style=\"font-family: '" + KGlobalSettings::fixedFont().family() + "'; font-size: " + QString::number(KGlobalSettings::fixedFont().pointSize()) + "pt;\">"); //FIXME: Font size seems to be too small
        text.append("</pre></body></html>");
    } else if (d->comboBox->currentIndex() < 9) {
        /// bibtex2html

        /// remove "generated by" line from HTML code if BibTeX2HTML was used
        text.replace(QRegExp("<hr><p><em>.*</p>"), "");
        text.replace(QRegExp("<[/]?(font)[^>]*>"), "");
        QRegExp reTable("^.*<td.*</td.*<td>");
        reTable.setMinimal(true);
        text.replace(reTable, "");
        text.replace(QRegExp("</td>.*$"), "");
        QRegExp reAnchor("\\[ <a.*</a> \\]");
        reAnchor.setMinimal(true);
        text.replace(reAnchor, "");

        text.prepend("<html><body style=\"font-family: '" + font().family() + "'; font-size: " + QString::number(font().pointSize()) + "pt;\">");
        text.append("</body></html>");
    } else {
        /// XML/XSLT
        text.prepend("<html><body style=\"font-family: '" + font().family() + "'; font-size: " + QString::number(font().pointSize()) + "pt;\">");
        text.append("</body></html>");
    }

    /// beautify text
    text.replace("``", "&ldquo;");
    text.replace("''", "&rdquo;");

    setHtml(text, d->baseUrl);

    d->saveState();

    QApplication::restoreOverrideCursor();
}

void ReferencePreview::openAsHTML()
{
    KTemporaryFile file;
    file.setSuffix(".html");
    file.setAutoRemove(false); /// let file stay alive for browser
    d->saveHTML(file);
    QDesktopServices::openUrl(KUrl(file.fileName()));
}

void ReferencePreview::saveAsHTML()
{
    KUrl url = KFileDialog::getSaveUrl(KUrl(), "text/html", this, i18n("Save as HTML"));
    if (url.isValid())
        d->saveHTML(url);
}
