#ifndef KBIBTEXOS_EXPORT_H
#define KBIBTEXOS_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXOS_EXPORT
# if defined(MAKE_ONLINESEARCH_LIB)
/* We are building this library */
#  define KBIBTEXOS_EXPORT KDE_EXPORT
# else // MAKE_ONLINESEARCH_LIB
/* We are using this library */
#  define KBIBTEXOS_EXPORT KDE_IMPORT
# endif // MAKE_ONLINESEARCH_LIB
#endif // KBIBTEXOS_EXPORT

#endif // KBIBTEXOS_EXPORT_H
