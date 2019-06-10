set(
    kbibtexconfig_LIB_SRCS
    notificationhub.cpp
    bibtexentries.cpp
    bibtexfields.cpp
    preferences.cpp
    logging_config.cpp
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexconfig kbibtexconfig_LIB_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexconfig
    SHARED
    ${kbibtexconfig_LIB_SRCS}
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
        Qt5::Core
        KBibTeX::Global
    PRIVATE
        KF5::ConfigCore
        KF5::ConfigGui
        KF5::I18n
)

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
