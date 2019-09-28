/***************************************************************************
 *   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_GUI_ENTRYLAYOUT_H
#define KBIBTEX_GUI_ENTRYLAYOUT_H

#include <QString>
#include <QVector>
#include <QSharedPointer>

#include <KBibTeX>

typedef struct {
    QString uiLabel;
    QString bibtexLabel;
    KBibTeX::FieldInputType fieldInputLayout;
} SingleFieldLayout;

typedef struct {
    QString identifier;
    QString uiCaption;
    QString iconName;
    int columns;
    QList<SingleFieldLayout> singleFieldLayouts;
    QStringList infoMessages;
} EntryTabLayout;

/**
@author Thomas Fischer
 */
class EntryLayout : public QVector<QSharedPointer<EntryTabLayout> >
{
public:
    virtual ~EntryLayout();

    static const EntryLayout &instance();

protected:
    explicit EntryLayout(const QString &style);

private:
    class EntryLayoutPrivate;
    EntryLayoutPrivate *d;
};

#endif // KBIBTEX_GUI_ENTRYLAYOUT_H
