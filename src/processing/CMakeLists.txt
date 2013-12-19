# Processing library

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
)

set(
    processing_LIB_SRCS
    findduplicates.cpp
    idsuggestions.cpp
    lyx.cpp
    checkbibtex.cpp
)

set(
    processing_HDRS
    findduplicates.h
    idsuggestions.h
    lyx.h
    checkbibtex.h
    kbibtexproc_export.h
)

add_definitions(
    -DMAKE_PROCESSING_LIB
)

# debug area for KBibTeX's processing library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101016
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexproc processing_LIB_SRCS)
endif(UNITY_BUILD)

kde4_add_library(
    kbibtexproc
    SHARED
    ${processing_LIB_SRCS}
)

target_link_libraries(
    kbibtexproc
    LINK_PRIVATE
    ${QT_QTCORE_LIBRARY}
    ${KDE4_KDECORE_LIBS}
    ${KDE4_KDEUI_LIBS}
    kbibtexconfig
    kbibtexdata
    kbibtexio
)

set_target_properties(
    kbibtexproc
    PROPERTIES
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexproc
    RUNTIME
    DESTINATION
    bin
    LIBRARY
    DESTINATION
    ${LIB_INSTALL_DIR}
)

install(
    FILES
    lyx.rc
    DESTINATION
    ${DATA_INSTALL_DIR}/kbibtex
)

install(
    FILES
    ${processing_HDRS}
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex
    COMPONENT
    Devel
)
