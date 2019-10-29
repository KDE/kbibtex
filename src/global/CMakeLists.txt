set(
    kbibtexglobal_SRCS
    kbibtex.cpp
)

ecm_qt_declare_logging_category(kbibtexglobal_SRCS
    HEADER logging_global.h
    IDENTIFIER LOG_KBIBTEX_GLOBAL
    CATEGORY_NAME kbibtex.global
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexglobal kbibtexglobal_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexglobal
    SHARED
    ${kbibtexglobal_SRCS}
)
generate_export_header(kbibtexglobal)
add_library(KBibTeX::Global ALIAS kbibtexglobal)

set_target_properties(
    kbibtexglobal
    PROPERTIES
    EXPORT_NAME "kbibtexglobal"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

target_include_directories(kbibtexglobal
    INTERFACE
        $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KBibTeX/global>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/global>
)

target_link_libraries(kbibtexglobal
    PUBLIC
        Qt5::Core
    PRIVATE
        KF5::ConfigCore
        KF5::I18n
)

install(
    TARGETS kbibtexglobal
    EXPORT kbibtexglobal-targets
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

set_target_properties(kbibtexglobal PROPERTIES
    EXPORT_NAME "Global"
)

ecm_generate_headers(kbibtexglobal_HEADERS
    HEADER_NAMES
        KBibTeX
    REQUIRED_HEADERS kbibtexglobal_HEADERS
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexglobal_export.h
    ${kbibtexglobal_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX/global
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGlobal-configVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/KBibTeXGlobal-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGlobal-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGlobal-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGlobal-configVersion.cmake
    DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)
