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

#include "referencepreview.h"

using namespace KBibTeX::Program;

const QString notAvailableMessage = "<html><body bgcolor=\"#fff\" fgcolor=\"#000\"><em>No preview available</em></body></html>";

ReferencePreview::ReferencePreview(QWidget *parent)
        : QWebView(parent)
{
    setEnabled(false);
}

void ReferencePreview::setHtml(const QString & html, const QUrl & baseUrl)
{
    m_htmlText = html;
    m_baseUrl = baseUrl;
    QWebView::setHtml(html, baseUrl);
}

void ReferencePreview::setEnabled(bool enabled)
{
    if (enabled)
        QWebView::setHtml(m_htmlText, m_baseUrl);
    else
        QWebView::setHtml(notAvailableMessage, m_baseUrl);
    QWebView::setEnabled(enabled);
}
