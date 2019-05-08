/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_PROGRAM_DOCKLET_REFERENCEPREVIEW_H
#define KBIBTEX_PROGRAM_DOCKLET_REFERENCEPREVIEW_H

#include <QWidget>
#include <QUrl>

class Element;
class File;
class FileView;

class ReferencePreview : public QWidget
{
    Q_OBJECT
public:
    explicit ReferencePreview(QWidget *parent);
    ~ReferencePreview() override;

    void setEnabled(bool);

    void setFileView(FileView *fileView);

public slots:
    void setElement(QSharedPointer<Element>, const File *);

private:
    class ReferencePreviewPrivate;
    ReferencePreviewPrivate *d;

    void setHtml(const QString &html, bool buttonsEnabled);

private slots:
    void renderHTML();
    void openAsHTML();
    void saveAsHTML();
    void linkClicked(const QUrl &);
};

#endif // KBIBTEX_PROGRAM_DOCKLET_REFERENCEPREVIEW_H
