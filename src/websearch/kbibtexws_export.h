#ifndef KBIBTEXWS_EXPORT_H
#define KBIBTEXWS_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXWS_EXPORT
# if defined(MAKE_WEBSEARCH_LIB)
/* We are building this library */
#  define KBIBTEXWS_EXPORT KDE_EXPORT
# else // MAKE_WEBSEARCH_LIB
/* We are using this library */
#  define KBIBTEXWS_EXPORT KDE_IMPORT
# endif // MAKE_WEBSEARCH_LIB
#endif // KBIBTEXWS_EXPORT

#endif // KBIBTEXWS_EXPORT_H
