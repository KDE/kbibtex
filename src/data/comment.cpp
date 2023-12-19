/***************************************************************************
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 *   SPDX-FileCopyrightText: 2004-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
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

#include "comment.h"

#include <typeinfo>

#include <QStringList>

#include "logging_data.h"

/**
 * Private class to store internal variables that should not be visible
 * in the interface as defined in the header file.
 */
class Comment::Private
{
public:
    QString text;
    Preferences::CommentContext context;
    // For use with context 'Prefix'
    QString prefix;

    Private(const QString &_text, Preferences::CommentContext _context, const QString &_prefix)
            : text(_text), context(_context), prefix(_prefix)
    {
        if (context != Preferences::CommentContext::Prefix)
            prefix.clear();
        else if (context == Preferences::CommentContext::Prefix && (prefix.isEmpty() || !prefix.startsWith(QStringLiteral("%")))) {
            qCDebug(LOG_KBIBTEX_DATA) << "Requested to create Comment with context 'prefix', but provided prefix was either empty or did not start with '%'. Changing comments context to 'Verbatim' and clearing prefix.";
            context = Preferences::CommentContext::Verbatim;
            prefix.clear();
        }
    }
};

Comment::Comment(const QString &text, Preferences::CommentContext context, const QString &prefix)
        : Element(), d(new Comment::Private(text, context, prefix))
{
    // nothing
}

Comment::Comment(const Comment &other)
        : Element(), d(new Comment::Private(other.d->text, other.d->context, other.d->prefix))
{
    // nothing
}

Comment::~Comment()
{
    delete d;
}

Comment &Comment::operator= (const Comment &other)
{
    if (this != &other) {
        d->text = other.d->text;
        d->context = other.d->context;
        d->prefix = other.d->prefix;
    }
    return *this;
}

QString Comment::text() const
{
    return d->text;
}

void Comment::setText(const QString &text)
{
    d->text = text;
}

Preferences::CommentContext Comment::context() const
{
    return d->context;
}

void Comment::setContext(Preferences::CommentContext context)
{
    d->context = context;
}

QString Comment::prefix() const
{
    return d->prefix;
}

void Comment::setPrefix(const QString &prefix)
{
    d->prefix = prefix;
}

bool Comment::isComment(const Element &other) {
    return typeid(other) == typeid(Comment);
}

bool Comment::operator==(const Comment &other) const
{
    return d->text == other.d->text && d->context == other.d->context && (d->context != Preferences::CommentContext::Prefix || d->prefix == other.d->prefix);
}

bool Comment::operator!=(const Comment &other) const
{
    return !operator==(other);
}

QDebug operator<<(QDebug dbg, const Comment &comment) {
    dbg.nospace() << "Comment " << comment.text();
    return dbg;
}
