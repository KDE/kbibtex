# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2012-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2013-2017 Pino Toscano <pino@kde.org>
# SPDX-FileContributor: 2019 Jonathan Riddell <jr@jriddell.org>

set(
    kbibtexio_SRCS
    encoder.cpp
    encoderlatex.cpp
    encoderxml.cpp
    fileexporterbibtex2html.cpp
    fileexporterbibtex.cpp
    fileexporterbibutils.cpp
    fileexporterbibtexoutput.cpp
    fileexporter.cpp
    fileexporterpdf.cpp
    fileexporterps.cpp
    fileexporterris.cpp
    fileexporterrtf.cpp
    fileexportertoolchain.cpp
    fileexporterwordbibxml.cpp
    fileexporterxml.cpp
    fileimporterbibtex.cpp
    fileimporterbibutils.cpp
    fileimporter.cpp
    fileimporterris.cpp
    fileinfo.cpp
    bibutils.cpp
)

if(${Poppler_FOUND})
    set(
        kbibtexio_SRCS
        ${kbibtexio_SRCS}
        fileimporterpdf.cpp
    )
endif()

ecm_qt_declare_logging_category(kbibtexio_SRCS
    HEADER logging_io.h
    IDENTIFIER LOG_KBIBTEX_IO
    CATEGORY_NAME kbibtex.io
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexio kbibtexio_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexio
    SHARED
    ${kbibtexio_SRCS}
)
generate_export_header(kbibtexio)
add_library(KBibTeX::IO ALIAS kbibtexio)

set_target_properties(kbibtexio
    PROPERTIES
    EXPORT_NAME "kbibtexio"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

target_include_directories(kbibtexio
    INTERFACE
        $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KBibTeX/io>
)

target_link_libraries(kbibtexio
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        KBibTeX::Data
        KBibTeX::Global
    PRIVATE
        KBibTeX::Config
        ICU::uc
        ICU::i18n
)

# FIXME get rid of Qt6Core5Compat
if(Qt6Core5Compat_FOUND)
    target_link_libraries(kbibtexio
        PRIVATE
            Qt6::Core5Compat
    )
endif()

if(KF${QT_MAJOR_VERSION}I18n_FOUND)
    target_link_libraries(kbibtexio
        PRIVATE
            KF${QT_MAJOR_VERSION}::I18n
    )
endif()

if(${Poppler_FOUND})
    target_link_libraries(kbibtexio
        PRIVATE
            Poppler::Qt${QT_MAJOR_VERSION}
    )
endif()

install(
    TARGETS kbibtexio
    EXPORT kbibtexio-targets
    LIBRARY NAMELINK_SKIP
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

set_target_properties(kbibtexio PROPERTIES
    EXPORT_NAME "IO"
)

ecm_generate_headers(kbibtexio_HEADERS
    HEADER_NAMES
        BibUtils
        Encoder
        EncoderLaTeX
        EncoderXML
        FileExporter
        FileExporterBibTeX
        FileExporterBibTeX2HTML
        FileExporterBibTeXOutput
        FileExporterBibUtils
        FileExporterPDF
        FileExporterPS
        FileExporterRIS
        FileExporterRTF
        FileExporterToolchain
        FileExporterXML
        FileImporter
        FileImporterBibTeX
        FileImporterBibUtils
        FileImporterRIS
        FileInfo
    REQUIRED_HEADERS kbibtexio_HEADERS
)

if(${Poppler_FOUND})
    ecm_generate_headers(kbibtexio_HEADERS
        HEADER_NAMES
            FileImporterPDF
        REQUIRED_HEADERS kbibtexio_HEADERS
    )
endif()

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexio_export.h
    ${kbibtexio_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX/io
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXIO-configVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/KBibTeXIO-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXIO-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXIO-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXIO-configVersion.cmake
    DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)
