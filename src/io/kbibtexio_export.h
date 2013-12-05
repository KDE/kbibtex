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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef KBIBTEXIO_EXPORT_H
#define KBIBTEXIO_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXIO_EXPORT
# if defined(MAKE_KBIBTEXIO_LIB)
/* We are building this library */
#  define KBIBTEXIO_EXPORT KDE_EXPORT
# else // MAKE_KBIBTEXIO_LIB
/* We are using this library */
#  define KBIBTEXIO_EXPORT KDE_IMPORT
# endif // MAKE_KBIBTEXIO_LIB
#endif // KBIBTEXIO_EXPORT

#endif // KBIBTEXIO_EXPORT_H
