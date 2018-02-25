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
    models/filemodel.cpp
    ${CMAKE_SOURCE_DIR}/src/global/kbibtex.cpp
    ${CMAKE_SOURCE_DIR}/src/global/preferences.cpp
    logging_data.cpp
)

set(
    kbibtexdata_HDRS
    comment.h
    element.h
    entry.h
    file.h
    macro.h
    preamble.h
    value.h
    models/filemodel.h
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexdata kbibtexdata_LIB_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/global
)

add_library(
    kbibtexdata
    SHARED
    ${kbibtexdata_LIB_SRCS}
)

target_link_libraries( kbibtexdata
    Qt5::Core
    Qt5::Widgets
    KF5::I18n
    KF5::XmlGui
    kbibtexconfig
)

set_target_properties(
    kbibtexdata
    PROPERTIES
    EXPORT_NAME "kbibtexdata"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

install(
    TARGETS
    kbibtexdata
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexdata )
