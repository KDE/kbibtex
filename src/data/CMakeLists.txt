# KBibTeXData library

set(
    kbibtexdata_SRCS
    comment.cpp
    element.cpp
    entry.cpp
    file.cpp
    macro.cpp
    preamble.cpp
    value.cpp
    models/filemodel.cpp
)

ecm_qt_declare_logging_category(kbibtexdata_SRCS
    HEADER logging_data.h
    IDENTIFIER LOG_KBIBTEX_DATA
    CATEGORY_NAME kbibtex.data
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexdata kbibtexdata_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexdata
    SHARED
    ${kbibtexdata_SRCS}
)
generate_export_header(kbibtexdata)
add_library(KBibTeX::Data ALIAS kbibtexdata)

set_target_properties(
    kbibtexdata
    PROPERTIES
    EXPORT_NAME "kbibtexdata"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

target_include_directories(kbibtexdata
    INTERFACE
        $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KBibTeX/data>
)

target_link_libraries(kbibtexdata
    PUBLIC
        Qt5::Core
        KBibTeX::Config
    PRIVATE
        KF5::ConfigCore
        KF5::I18n
        KBibTeX::Global
)

install(
    TARGETS kbibtexdata
    EXPORT kbibtexdata-targets
    LIBRARY NAMELINK_SKIP
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

set_target_properties(kbibtexdata PROPERTIES
    EXPORT_NAME "Data"
)

ecm_generate_headers(kbibtexdata_HEADERS
    HEADER_NAMES
        Comment
        Element
        Entry
        File
        Macro
        Preamble
        Value
        models/FileModel
    REQUIRED_HEADERS kbibtexdata_HEADERS
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexdata_export.h
    ${kbibtexdata_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX/data
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXData-configVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/KBibTeXData-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXData-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXData-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXData-configVersion.cmake
    DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)
