# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2012-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2019 Jonathan Riddell <jr@jriddell.org>

set(
    kbibtexconfig_SRCS
    notificationhub.cpp
    bibtexentries.cpp
    bibtexfields.cpp
    preferences.cpp
)

ecm_qt_declare_logging_category(kbibtexconfig_SRCS
    HEADER logging_config.h
    IDENTIFIER LOG_KBIBTEX_CONFIG
    CATEGORY_NAME kbibtex.config
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexconfig kbibtexconfig_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexconfig
    SHARED
    ${kbibtexconfig_SRCS}
)

generate_export_header(kbibtexconfig)
add_library(KBibTeX::Config ALIAS kbibtexconfig)

set_target_properties(kbibtexconfig
    PROPERTIES
    EXPORT_NAME "kbibtexconfig"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

target_include_directories(kbibtexconfig
    INTERFACE
        $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KBibTeX/config>
)

target_link_libraries(kbibtexconfig
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        KBibTeX::Global
    PRIVATE
        KF${QT_MAJOR_VERSION}::ConfigCore
)

if(KF${QT_MAJOR_VERSION}I18n_FOUND)
    target_link_libraries(kbibtexconfig
        PRIVATE
            KF${QT_MAJOR_VERSION}::I18n
    )
endif()

install(
    TARGETS kbibtexconfig
    EXPORT kbibtexconfig-targets
    LIBRARY NAMELINK_SKIP
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

set_target_properties(kbibtexconfig PROPERTIES
    EXPORT_NAME "Config"
)

ecm_generate_headers(kbibtexconfig_HEADERS
    HEADER_NAMES
        BibTeXEntries
        BibTeXFields
        NotificationHub
        Preferences
    REQUIRED_HEADERS kbibtexconfig_HEADERS
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexconfig_export.h
    ${kbibtexconfig_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX/config
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXConfig-configVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/KBibTeXConfig-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXConfig-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXConfig-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXConfig-configVersion.cmake
    DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)
