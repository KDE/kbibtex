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
