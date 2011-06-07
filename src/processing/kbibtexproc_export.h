#ifndef KBIBTEXPROC_EXPORT_H
#define KBIBTEXPROC_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXPROC_EXPORT
# if defined(MAKE_KBIBTEXPROC_LIB)
/* We are building this library */
#  define KBIBTEXPROC_EXPORT KDE_EXPORT
# else // MAKE_KBIBTEXPROC_LIB
/* We are using this library */
#  define KBIBTEXPROC_EXPORT KDE_IMPORT
# endif // MAKE_KBIBTEXPROC_LIB
#endif // KBIBTEXPROC_EXPORT

#endif // KBIBTEXPROC_EXPORT_H
