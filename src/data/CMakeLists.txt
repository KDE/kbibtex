# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2012-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2013 Pino Toscano <pino@kde.org>
# SPDX-FileContributor: 2019 Jonathan Riddell <jr@jriddell.org>

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
)

if(Qt${QT_MAJOR_VERSION}Gui_FOUND)
    set(
        kbibtexdata_SRCS
        ${kbibtexdata_SRCS}
        models/filemodel.cpp
    )
endif()

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
        Qt${QT_MAJOR_VERSION}::Core
        KBibTeX::Config
    PRIVATE
        KF${QT_MAJOR_VERSION}::ConfigCore
        KBibTeX::Global
)

if(KF${QT_MAJOR_VERSION}I18n_FOUND)
    target_link_libraries(kbibtexdata
        PRIVATE
            KF${QT_MAJOR_VERSION}::I18n
    )
endif()

if(BUILD_KPART OR BUILD_TESTING)
    target_link_libraries(kbibtexdata
        PRIVATE
            Qt${QT_MAJOR_VERSION}::Gui
    )
endif()

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
    REQUIRED_HEADERS kbibtexdata_HEADERS
)

if(Qt${QT_MAJOR_VERSION}Gui_FOUND)
    ecm_generate_headers(kbibtexdata_HEADERS
        HEADER_NAMES
            models/FileModel
        REQUIRED_HEADERS kbibtexdata_HEADERS
    )
endif()

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
