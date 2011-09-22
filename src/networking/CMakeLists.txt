# OnlineSearch library

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/onlinesearch
    ${POPPLER_QT4_INCLUDE_DIR}
)

set( networking_LIB_SRCS
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
    onlinesearch/onlinesearchgeneral.cpp
    findpdf.cpp
    httpequivcookiejar.cpp
)

add_definitions( -DMAKE_NETWORKING_LIB )

# debug area for KBibTeX's web search library
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101025)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
)

kde4_add_library( kbibtexnetworking SHARED ${networking_LIB_SRCS} )

target_link_libraries( kbibtexnetworking
    ${QT_QTCORE_LIBRARY}
    ${QT_QTWEBKIT_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${POPPLER_QT4_LIBRARIES}
    kbibtexio
)

install(TARGETS kbibtexnetworking RUNTIME DESTINATION bin LIBRARY DESTINATION ${LIB_INSTALL_DIR})

