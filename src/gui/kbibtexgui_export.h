#ifndef KBIBTEXGUI_EXPORT_H
#define KBIBTEXGUI_EXPORT_H

#include <kdemacros.h>

#ifndef KBIBTEXGUI_EXPORT
# if defined(MAKE_KBIBTEXGUI_LIB)
/* We are building this library */
#  define KBIBTEXGUI_EXPORT KDE_EXPORT
# else // MAKE_KBIBTEXGUI_LIB
/* We are using this library */
#  define KBIBTEXGUI_EXPORT KDE_IMPORT
# endif // MAKE_KBIBTEXGUI_LIB
#endif // KBIBTEXGUI_EXPORT

#endif // KBIBTEXGUI_EXPORT_H
