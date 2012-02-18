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
#ifndef KBIBTEX_IO_COMMENT_H
#define KBIBTEX_IO_COMMENT_H

#include "element.h"

/**
 * This class represents a comment in a BibTeX file. In BibTeX files,
 * everything that cannot be interpreted as a BibTeX comment is see
 * as a comment. Alternatively, the comment command can be used in BibTeX
 * files.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT Comment : public Element
{
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(bool useCommand READ useCommand WRITE setUseCommand)

public:
    /**
     * Create a new comment with a given text.
     * @param text comment's textual content
     * @param useCommand mark this comment to use BibTeX's comment command
     */
    Comment(const QString &text = QString::null, bool useCommand = false);

    /**
     * Copy constructor cloning another comment object.
     * @param other comment object to clone
     */
    Comment(const Comment& other);

    virtual ~Comment();

    /**
     * Retrieve the text of this comment.
     * @return text of this comment
     */
    QString text() const;

    /**
     * Set the text of this comment.
     * @param text text of this comment
     */
    void setText(const QString &text);

    /**
     * Retrieve the flag whether to use BibTeX's comment command or not.
     * @return mark if this comment has to use BibTeX's comment command
     */
    bool useCommand() const;

    /**
     * Set the flag whether to use BibTeX's comment command or not.
     * @param useCommand set if this comment has to use BibTeX's comment command
     */
    void setUseCommand(bool useCommand);

private:
    class CommentPrivate;
    CommentPrivate * const d;
};

#endif // KBIBTEX_IO_COMMENT_H
