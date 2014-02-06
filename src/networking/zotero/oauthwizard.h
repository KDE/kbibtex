/***************************************************************************
 *   Copyright (C) 2004-2013 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#ifndef KBIBTEX_NETWORKING_ZOTERO_OAUTHWIZARD_H
#define KBIBTEX_NETWORKING_ZOTERO_OAUTHWIZARD_H

#ifdef HAVE_QTOAUTH

#include <QWizard>

#include "kbibtexnetworking_export.h"

namespace Zotero
{

/**
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXNETWORKING_EXPORT OAuthWizard : public QWizard
{
    Q_OBJECT

public:
    explicit OAuthWizard(QWidget *parent);
    ~OAuthWizard();

    bool exec();

    int userId() const;
    QString apiKey() const;
    QString username() const;

protected:
    virtual void initializePage(int id);
    virtual void accept();

private slots:
    void copyAuthorizationUrl();
    void openAuthorizationUrl();

private:
    class Private;
    Private *const d;
};

} // end of namespace Zotero

#endif // HAVE_QTOAUTH

#endif // KBIBTEX_NETWORKING_ZOTERO_OAUTHWIZARD_H
