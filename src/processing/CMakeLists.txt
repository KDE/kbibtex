# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2011-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2019 Jonathan Riddell <jr@jriddell.org>

set(
    kbibtexprocessing_SRCS
    idsuggestions.cpp
    journalabbreviations.cpp
)

if(BUILD_KPART OR BUILD_TESTING)
    set(
        kbibtexprocessing_SRCS
        ${kbibtexprocessing_SRCS}
        findduplicates.cpp
        lyx.cpp
        checkbibtex.cpp
        bibliographyservice.cpp
    )
endif()

ecm_qt_declare_logging_category(kbibtexprocessing_SRCS
    HEADER logging_processing.h
    IDENTIFIER LOG_KBIBTEX_PROCESSING
    CATEGORY_NAME kbibtex.processing
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexproc kbibtexprocessing_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexprocessing
    SHARED
    ${kbibtexprocessing_SRCS}
)
generate_export_header(kbibtexprocessing)
add_library(KBibTeX::Processing ALIAS kbibtexprocessing)

set_target_properties(kbibtexprocessing
    PROPERTIES
    EXPORT_NAME "kbibtexprocessing"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

target_include_directories(kbibtexprocessing
    INTERFACE
        $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KBibTeX/processing>
)

target_link_libraries(kbibtexprocessing
    PUBLIC
        Qt5::Core
        KBibTeX::Data
    PRIVATE
        KF5::ConfigCore
        KBibTeX::Config
        KBibTeX::IO
)

if(BUILD_KPART OR BUILD_TESTING)
    target_link_libraries(kbibtexprocessing
        PRIVATE
            Qt5::Widgets
            KF5::WidgetsAddons
            KF5::XmlGui
            KF5::Parts
    )
endif()

if(KF5I18n_FOUND)
    target_link_libraries(kbibtexprocessing
        PRIVATE
            KF5::I18n
    )
endif()

install(
    TARGETS kbibtexprocessing
    EXPORT kbibtexprocessing-targets
    LIBRARY NAMELINK_SKIP
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

set_target_properties(kbibtexprocessing PROPERTIES
    EXPORT_NAME "Processing"
)

ecm_generate_headers(kbibtexprocessing_HEADERS
    HEADER_NAMES
        IdSuggestions
        JournalAbbreviations
    REQUIRED_HEADERS kbibtexprocessing_HEADERS
)

if(BUILD_KPART OR BUILD_TESTING)
    ecm_generate_headers(kbibtexprocessing_HEADERS
        HEADER_NAMES
            BibliographyService
            FindDuplicates
            CheckBibTeX
            LyX
        REQUIRED_HEADERS kbibtexprocessing_HEADERS
    )
endif()

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexprocessing_export.h
    ${kbibtexprocessing_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX/processing
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXProcessing-configVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/KBibTeXProcessing-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXProcessing-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXProcessing-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXProcessing-configVersion.cmake
    DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)
