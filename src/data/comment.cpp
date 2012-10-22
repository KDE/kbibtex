/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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
#include <QRegExp>
#include <QStringList>

#include "comment.h"

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class Comment::CommentPrivate
{
public:
    QString text;
    bool useCommand;
};

Comment::Comment(const QString &text, bool useCommand)
        : Element(), d(new Comment::CommentPrivate)
{
    d->text = text;
    d->useCommand = useCommand;
}

Comment::Comment(const Comment &other)
        : Element(), d(new Comment::CommentPrivate)
{
    d->text = other.d->text;
    d->useCommand = other.d->useCommand;
}

Comment::~Comment()
{
    delete d;
}

QString Comment::text() const
{
    return d->text;
}

void Comment::setText(const QString &text)
{
    d->text = text;
}

bool Comment::useCommand() const
{
    return d->useCommand;
}

void Comment::setUseCommand(bool useCommand)
{
    d->useCommand = useCommand;
}
