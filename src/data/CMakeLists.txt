# KBibTeXData library

set(
    kbibtexdata_LIB_SRCS
    comment.cpp
    element.cpp
    entry.cpp
    file.cpp
    macro.cpp
    preamble.cpp
    value.cpp
)

set(
    kbibtexdata_HDRS
    comment.h
    element.h
    entry.h
    file.h
    kbibtexdata_export.h
    macro.h
    preamble.h
    value.h
)

add_definitions(
    -DMAKE_KBIBTEXDATA_LIB
)

# debug area for KBibTeX's Data library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101032
)

include_directories(
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${POPPLER_QT4_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_SOURCE_DIR}/src/config
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexdata kbibtexdata_LIB_SRCS)
endif(UNITY_BUILD)

kde4_add_library(
    kbibtexdata
    SHARED
    ${kbibtexdata_LIB_SRCS}
)

target_link_libraries(
    kbibtexdata
    LINK_PRIVATE
    ${QT_QTCORE_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KIO_LIBS}
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${POPPLER_QT4_LIBRARIES}
    kbibtexconfig
)

if(
    WIN32
)
    target_link_libraries(
        kbibtexdata
        LINK_PRIVATE
        iconv
    )
endif(
    WIN32
)

set_target_properties(
    kbibtexdata
    PROPERTIES
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexdata
    RUNTIME
    DESTINATION
    bin
    LIBRARY
    DESTINATION
    ${LIB_INSTALL_DIR}
)

install(
    FILES
    ${kbibtexdata_HDRS}
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex
    COMPONENT
    Devel
)
