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
