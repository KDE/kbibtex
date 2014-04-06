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

#ifndef KBIBTEXCONFIG_EXPORT_H
#define KBIBTEXCONFIG_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXCONFIG_EXPORT
# if defined(MAKE_KBIBTEXCONFIG_LIB)
/* We are building this library */
#  define KBIBTEXCONFIG_EXPORT KDE_EXPORT
# else // MAKE_KBIBTEXCONFIG_LIB
/* We are using this library */
#  define KBIBTEXCONFIG_EXPORT KDE_IMPORT
# endif // MAKE_KBIBTEXCONFIG_LIB
#endif // KBIBTEXCONFIG_EXPORT

#endif // KBIBTEXCONFIG_EXPORT_H