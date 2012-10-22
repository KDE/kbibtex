#ifndef KBIBTEXDATA_EXPORT_H
#define KBIBTEXDATA_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXDATA_EXPORT
# if defined(MAKE_KBIBTEXDATA_LIB)
/* We are building this library */
#  define KBIBTEXDATA_EXPORT KDE_EXPORT
# else // MAKE_KBIBTEXDATA_LIB
/* We are using this library */
#  define KBIBTEXDATA_EXPORT KDE_IMPORT
# endif // MAKE_KBIBTEXDATA_LIB
#endif // KBIBTEXDATA_EXPORT

#endif // KBIBTEXDATA_EXPORT_H
