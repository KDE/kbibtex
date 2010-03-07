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

#include <QBuffer>
#include <QWebView>
#include <QLayout>
#include <QApplication>

#include <KLocale>
#include <KComboBox>
#include <KStandardDirs>

#include <fileexporterbibtex.h>
#include <fileexporterbibtex2html.h>
#include <fileexporterxslt.h>

#include "referencepreview.h"

using namespace KBibTeX::Program;

const QString notAvailableMessage = "<html><body style=\"font-family: '" + KGlobalSettings::generalFont().family() + "'; font-size: " + QString::number(KGlobalSettings::generalFont().pointSize()) + "pt; font-style: italic; color: #333;\">" + i18n("No preview available") + "</body></html>"; //FIXME: Font size seems to be too small
class ReferencePreview::ReferencePreviewPrivate
{
private:
    ReferencePreview *p;

public:
    QString htmlText;
    QUrl baseUrl;
    QWebView *webView;
    KComboBox *comboBox;
    const KBibTeX::IO::Element* element;

    ReferencePreviewPrivate(ReferencePreview *parent)
            : p(parent), element(NULL) {
        QVBoxLayout *layout = new QVBoxLayout(p);
        layout->setMargin(0);
        comboBox = new KComboBox(p);
        layout->addWidget(comboBox);
        webView = new QWebView(p);
        layout->addWidget(webView);

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
        connect(comboBox, SIGNAL(currentIndexChanged(int)), p, SLOT(renderHTML()));
    }

};

ReferencePreview::ReferencePreview(QWidget *parent)
        : QWidget(parent), d(new ReferencePreviewPrivate(this))
{
    setEnabled(false);
}

void ReferencePreview::setHtml(const QString & html, const QUrl & baseUrl)
{
    d->htmlText = html;
    d->baseUrl = baseUrl;
    d->webView->setHtml(html, baseUrl);
}

void ReferencePreview::setEnabled(bool enabled)
{
    if (enabled)
        d->webView->setHtml(d->htmlText, d->baseUrl);
    else
        d->webView->setHtml(notAvailableMessage, d->baseUrl);
    d->webView->setEnabled(enabled);
    d->comboBox->setEnabled(enabled);
}

void ReferencePreview::setElement(const KBibTeX::IO::Element* element)
{
    d->element = element;
    renderHTML();
}

void ReferencePreview::renderHTML()
{
    if (d->element == NULL) {
        d->webView->setHtml(notAvailableMessage, d->baseUrl);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QStringList errorLog;
    KBibTeX::IO::FileExporter *exporter = NULL;

    if (d->comboBox->currentIndex() == 0)
        exporter = new KBibTeX::IO::FileExporterBibTeX();
    else if (d->comboBox->currentIndex() < 9) {
        KBibTeX::IO::FileExporterBibTeX2HTML *exporterHTML = new KBibTeX::IO::FileExporterBibTeX2HTML();
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
        KBibTeX::IO::FileExporterXSLT *exporterXSLT = new KBibTeX::IO::FileExporterXSLT();
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
    exporter->save(&buffer, d->element, &errorLog);
    buffer.close();
    delete exporter;

    buffer.open(QBuffer::ReadOnly);
    QTextStream ts(&buffer);
    QString text = ts.readAll();
    buffer.close();

    if (text.isEmpty()) /// something went wrong, no output ...
        text = notAvailableMessage;

    if (d->comboBox->currentIndex() == 0) {
        /// source
        text.prepend("<html><body><pre style=\"font-family: '" + KGlobalSettings::fixedFont().family() + "'; font-size: " + QString::number(KGlobalSettings::fixedFont().pointSize()) + "pt;\">"); //FIXME: Font size seems to be too small
        text.append("</pre></body></html>");
    } else if (d->comboBox->currentIndex() < 9) {
        /// bibtex2html

        /// remove "generated by" line from HTML code if BibTeX2HTML was used
        text.replace(QRegExp("<hr><p><em>.*</p>"), "");

        text.prepend("<html><body style=\"font-family: '" + font().family() + "'; font-size: " + QString::number(font().pointSize()) + "pt;\">");
        text.append("</body></html>");
    } else {
        /// XML/XSLT
/// nothing to do
    }

    d->webView->setHtml(text);

    QApplication::restoreOverrideCursor();
}
