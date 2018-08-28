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

#ifndef KBIBTEX_GUI_FINDDUPLICATESUI_H
#define KBIBTEX_GUI_FINDDUPLICATESUI_H

#include "kbibtexgui_export.h"

#include <QObject>
#include <QTreeView>

namespace KParts
{
class Part;
}
class QPushButton;

class FileView;
class EntryClique;
class File;

class RadioButtonTreeView;
class AlternativesItemModel;

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT MergeWidget : public QWidget
{
    Q_OBJECT

public:
    MergeWidget(File *file, QVector<EntryClique *> &cliques, QWidget *parent);
    ~MergeWidget() override;

    void showCurrentClique();

private slots:
    void previousClique();
    void nextClique();

private:
    class MergeWidgetPrivate;
    MergeWidgetPrivate *d;
};


/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXGUI_EXPORT FindDuplicatesUI : public QObject
{
    Q_OBJECT

public:
    FindDuplicatesUI(KParts::Part *part, FileView *fileView);
    ~FindDuplicatesUI() override;

private slots:
    void startDuplicatesSearch();

private:
    class FindDuplicatesUIPrivate;
    FindDuplicatesUIPrivate *d;
};

#endif // KBIBTEX_GUI_FINDDUPLICATESUI_H
