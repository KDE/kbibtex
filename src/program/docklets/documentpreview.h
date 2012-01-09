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

#ifndef KBIBTEX_PROGRAM_DOCUMENTPREVIEW_H
#define KBIBTEX_PROGRAM_DOCUMENTPREVIEW_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>

#include <KUrl>

class QDockWidget;
class QResizeEvent;

class KJob;
namespace KIO
{
class Job;
}

class Element;
class File;

class ImageLabel : public QLabel
{
public:
    ImageLabel(const QString &text, QWidget *parent = NULL, Qt::WindowFlags f = 0);
    void setPixmap(const QPixmap &pixmap);

protected:
    void resizeEvent(QResizeEvent *event);

private:
    QPixmap m_pixmap;
};

class DocumentPreview : public QWidget
{
    Q_OBJECT
public:
    DocumentPreview(QDockWidget *parent);
    ~DocumentPreview();

public slots:
    void setElement(QSharedPointer<Element>, const File *);
    void setBibTeXUrl(const KUrl&);

private:
    class DocumentPreviewPrivate;
    DocumentPreviewPrivate *d;

    QString mimeType(const KUrl &url);

private slots:
    void openExternally();
    void onlyLocalFilesChanged();
    void visibilityChanged(bool);
    void comboBoxChanged(int);
    void statFinished(KJob*);
    void loadingFinished();
    void linkActivated(const QString &link);
};

#endif // KBIBTEX_PROGRAM_DOCUMENTPREVIEW_H
