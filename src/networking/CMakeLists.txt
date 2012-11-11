# OnlineSearch library

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/networking/onlinesearch
    ${POPPLER_QT4_INCLUDE_DIR}
    ${LIBXML2_INCLUDE_DIR}
)

set(
    networking_LIB_SRCS
    onlinesearch/onlinesearchabstract.cpp
    onlinesearch/onlinesearchbibsonomy.cpp
    onlinesearch/onlinesearcharxiv.cpp
    onlinesearch/onlinesearchsciencedirect.cpp
    onlinesearch/onlinesearchgooglescholar.cpp
    onlinesearch/onlinesearchieeexplore.cpp
    onlinesearch/onlinesearchpubmed.cpp
    onlinesearch/onlinesearchacmportal.cpp
    onlinesearch/onlinesearchspringerlink.cpp
    onlinesearch/onlinesearchjstor.cpp
    onlinesearch/onlinesearchmathscinet.cpp
    onlinesearch/onlinesearchinspirehep.cpp
    onlinesearch/onlinesearchingentaconnect.cpp
    onlinesearch/onlinesearchsimplebibtexdownload.cpp
    onlinesearch/onlinesearchgeneral.cpp
    onlinesearch/onlinesearchsoanasaads.cpp
    onlinesearch/onlinesearchisbndb.cpp
    findpdf.cpp
    internalnetworkaccessmanager.cpp
)

set(
    kbibtexnetworking_HDRS
    onlinesearch/onlinesearchgeneral.h
    onlinesearch/onlinesearchsciencedirect.h
    onlinesearch/onlinesearchabstract.h
    onlinesearch/onlinesearchacmportal.h
    onlinesearch/onlinesearchbibsonomy.h
    onlinesearch/onlinesearchgooglescholar.h
    onlinesearch/onlinesearchspringerlink.h
    onlinesearch/onlinesearchjstor.h
    onlinesearch/onlinesearchieeexplore.h
    onlinesearch/onlinesearcharxiv.h
    onlinesearch/onlinesearchpubmed.h
    onlinesearch/onlinesearchingentaconnect.h
    onlinesearch/onlinesearchsimplebibtexdownload.h
    onlinesearch/onlinesearchsoanasaads.h
    onlinesearch/onlinesearchmathscinet.h
    onlinesearch/onlinesearchinspirehep.h
    onlinesearch/onlinesearchisbndb.h
    kbibtexnetworking_export.h
    findpdf.h
    internalnetworkaccessmanager.h
)

add_definitions(
    -DMAKE_NETWORKING_LIB
)

# debug area for KBibTeX's web search library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101025
)

kde4_add_library(
    kbibtexnetworking
    SHARED
    ${networking_LIB_SRCS}
)

target_link_libraries(
    kbibtexnetworking
    ${QT_QTCORE_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${POPPLER_QT4_LIBRARIES}
    kbibtexconfig
    kbibtexdata
    kbibtexio
)

set_target_properties(
    kbibtexnetworking
    PROPERTIES
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexnetworking
    RUNTIME
    DESTINATION
    bin
    LIBRARY
    DESTINATION
    ${LIB_INSTALL_DIR}
)

install(
    FILES
    ${kbibtexnetworking_HDRS}
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex
    COMPONENT
    Devel
)
