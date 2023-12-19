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

#ifndef KBIBTEX_DATA_COMMENT_H
#define KBIBTEX_DATA_COMMENT_H

#include <Element>
#include <Preferences>
#ifdef HAVE_KF
#include "kbibtexdata_export.h"
#endif // HAVE_KF

/**
 * This class represents a comment in a BibTeX file. In BibTeX files,
 * everything that cannot be interpreted as a BibTeX comment is see
 * as a comment. Alternatively, the comment command can be used in BibTeX
 * files.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT Comment : public Element
{
public:
    /**
     * Create a new comment with a given text.
     * @param text comment's textual content
     * @param useCommand mark this comment to use BibTeX's comment command
     */
    explicit Comment(const QString &text, Preferences::CommentContext context, const QString &prefix = QString());

    /**
     * Copy constructor cloning another comment object.
     * @param other comment object to clone
     */
    Comment(const Comment &other);

    ~Comment() override;

    /**
     * Assignment operator, working similar to a copy constructor,
     * but overwrites the current object's values.
     */
    Comment &operator= (const Comment &other);

    /**
     * Retrieve the text of this comment with out the prefix in case the context is 'Prefix'.
     * @return text of this comment
     */
    QString text() const;

    /**
     * Set the text of this comment (without the prefix in case the context is 'Prefix').
     * @param text text of this comment
     */
    void setText(const QString &text);

    /**
     * Retrieve the context of this comment
     * @return context of this comment
     */
    Preferences::CommentContext context() const;

    /**
     * Set the context of this comment
     * @param context the context to use for this comment
     */
    void setContext(const Preferences::CommentContext context);

    /**
     * Set the prefix of this comment.
     * Invoking this function will change this comment's prefix to 'Prefix'.
     * @param prefix the prefix to use for this comment
     */
    void setPrefix(const QString &prefix);

    /**
     * Retrieve the prefix of this comment.
     * If the prefix uses another prefix, this function will return QString().
     * @return prefix of this comment
     */
    QString prefix() const;

    /**
     * Cheap and fast test if another Element is a Comment object.
     * @param other another Element object to test
     * @return true if Element is actually a Comment
     */
    static bool isComment(const Element &other);

    bool operator==(const Comment &other) const;
    bool operator!=(const Comment &other) const;

private:
    class Private;
    Private *const d;
};

KBIBTEXDATA_EXPORT QDebug operator<<(QDebug dbg, const Comment &comment);

#endif // KBIBTEX_DATA_COMMENT_H
