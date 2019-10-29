set(
    kbibtexprocessing_SRCS
    findduplicates.cpp
    idsuggestions.cpp
    lyx.cpp
    checkbibtex.cpp
    bibliographyservice.cpp
    journalabbreviations.cpp
)

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
        Qt5::Widgets
        KF5::ConfigCore
        KF5::I18n
        KF5::Parts
        KF5::WidgetsAddons
        KF5::XmlGui
        KBibTeX::Config
        KBibTeX::IO
)

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
        BibliographyService
        CheckBibTeX
        FindDuplicates
        IdSuggestions
        JournalAbbreviations
        LyX
    REQUIRED_HEADERS kbibtexprocessing_HEADERS
)

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
