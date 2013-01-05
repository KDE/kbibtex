project(
    kbibtex
)

cmake_minimum_required(
    VERSION
    2.6
)

# Version number for the exported headers / libs
set(
    LIB_VERSION "0.5.0"
)
set(
    LIB_SOVERSION "1"
)

set(
    MANDIR
    "share/man"
    CACHE
    STRING
    "Where to install manpages"
)

find_package(
    Qt4
    REQUIRED
)
find_package(
    KDE4
    REQUIRED
)

# FIXME may have to be cleaned up a little bit
# Contributed by Jeremy Cribb <cribb.jeremy@gmail.com>
if(
    APPLE
)
    find_library(
        SYS_CONFIG_LIBRARY
        SystemConfiguration
    )
    mark_as_advanced(
        SYS_CONFIG_LIBRARY
    )
    # SET(TARGET_EXTERNAL_LIBRARIES iconv ${SYS_CONFIG_LIBRARY})
    set(
        ICONV_INCLUDE_DIR
        "/opt/local/include"
    )
    set(
        ICONV_LIBRARIES
        "/opt/local/lib/libiconv.dylib"
    )
    set(
        LIBXSLT_LIBRARIES
        "/opt/local/lib/libxslt.dylib"
    )
endif(
    APPLE
)

find_package(
    LibXslt
    REQUIRED
)
macro_log_feature(
    LIBXSLT_FOUND
    "LibXSLT"
    "A library to transform XML files into other XML files"
    "http://xmlsoft.org/XSLT"
    TRUE
    ""
    "Required to transform XML files"
)

find_package(
    LibXml2
    REQUIRED
)
macro_log_feature(
    LIBXML2_FOUND
    "LibXML2"
    "Libraries used to develop XML applications"
    "http://xmlsoft.org"
    TRUE
    ""
    "Required to transform XML files"
)

find_package(
    PopplerQt4
    REQUIRED
)
macro_log_feature(
    PopplerQt4_FOUND
    "Poppler/Qt4"
    "Poppler is a PDF rendering library"
    "http://poppler.freedesktop.org/"
    TRUE
    ""
    "Required to transform process PDF files"
)

find_path(
    QXTCORE_INCLUDE_DIRS
    qxtglobal.h
    PATH_SUFFIXES
    QxtCore
    DOC "The QxtCore library"
)
find_path(
    QXTGUI_INCLUDE_DIRS
    qxtglobalshortcut.h
    PATH_SUFFIXES
    QxtGui
    DOC "The QxtGui library"
)
set(
    QXT_INCLUDE_DIRS
    ${QXTCORE_INCLUDE_DIRS}
    ${QXTGUI_INCLUDE_DIRS}
)
find_library(
    QXT_LIBRARIES
    QxtGui
)

if(EXISTS ${QXT_LIBRARIES})
    message(
        STATUS "Found Qxt library dir: "
        ${QXT_LIBRARIES}
    )
    message(
        STATUS "Found Qxt include dirs: "
        ${QXT_INCLUDE_DIRS}
    )
else(EXISTS ${QXT_LIBRARIES})
    message(
        FATAL_ERROR "Looking for QxtGui - not found"
    )
endif(EXISTS ${QXT_LIBRARIES})


add_subdirectory(
    src
)
add_subdirectory(
    config
)
add_subdirectory(
    xslt
)
add_subdirectory(
    icons
)
add_subdirectory(
    man
)
add_subdirectory(
    mime
)
if(WIN32)
    message(
        "Translations are not yet supported on Windows platforms."
    )
else(WIN32)
    add_subdirectory(
        translations
    )
endif(WIN32)
