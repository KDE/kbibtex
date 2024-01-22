# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2011-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2014 Christophe Giboudeaux <cgiboudeaux@gmx.com>
# SPDX-FileContributor: 2014 Pavel Zorin-Kranich <pzorin@math.uni-bonn.de>
# SPDX-FileContributor: 2016 Christoph Feck <cfeck@kde.org>
# SPDX-FileContributor: 2017 Pino Toscano <pino@kde.org>
# SPDX-FileContributor: 2019 Jonathan Riddell <jr@jriddell.org>

set(
    kbibtexnetworking_SRCS
    onlinesearch/onlinesearchabstract.cpp
    onlinesearch/onlinesearchbibsonomy.cpp
    onlinesearch/onlinesearcharxiv.cpp
    onlinesearch/onlinesearchsciencedirect.cpp
    onlinesearch/onlinesearchgooglescholar.cpp
    onlinesearch/onlinesearchieeexplore.cpp
    onlinesearch/onlinesearchpubmed.cpp
    onlinesearch/onlinesearchacmportal.cpp
    onlinesearch/onlinesearchspringerlink.cpp
    onlinesearch/onlinesearchmathscinet.cpp
    onlinesearch/onlinesearchmrlookup.cpp
    onlinesearch/onlinesearchinspirehep.cpp
    onlinesearch/onlinesearchcernds.cpp
    onlinesearch/onlinesearchingentaconnect.cpp
    onlinesearch/onlinesearchsimplebibtexdownload.cpp
    onlinesearch/onlinesearchgeneral.cpp
    onlinesearch/onlinesearchsoanasaads.cpp
    onlinesearch/onlinesearchideasrepec.cpp
    onlinesearch/onlinesearchdoi.cpp
    onlinesearch/onlinesearchbiorxiv.cpp
    onlinesearch/onlinesearchsemanticscholar.cpp
    onlinesearch/onlinesearchunpaywall.cpp
    onlinesearch/onlinesearchzbmath.cpp
    zotero/api.cpp
    zotero/collectionmodel.cpp
    zotero/collection.cpp
    zotero/items.cpp
    zotero/groups.cpp
    zotero/oauthwizard.cpp
    zotero/tags.cpp
    zotero/tagmodel.cpp
    associatedfiles.cpp
    findpdf.cpp
    faviconlocator.cpp
    internalnetworkaccessmanager.cpp
    urlchecker.cpp
)

if(Qt${QT_MAJOR_VERSION}WebEngineWidgets_FOUND)
    # See also below for a similar if-clause
    message(STATUS "Using QtWebEngine parse JStor pages")
    set(
        kbibtexnetworking_SRCS
        ${kbibtexnetworking_SRCS}
        onlinesearch/onlinesearchjstor.cpp
    )
endif()

ecm_qt_declare_logging_category(kbibtexnetworking_SRCS
    HEADER logging_networking.h
    IDENTIFIER LOG_KBIBTEX_NETWORKING
    CATEGORY_NAME kbibtex.networking
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexnetworking kbibtexnetworking_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexnetworking
    SHARED
    ${kbibtexnetworking_SRCS}
)
generate_export_header(kbibtexnetworking)
add_library(KBibTeX::Networking ALIAS kbibtexnetworking)

set_target_properties(kbibtexnetworking
    PROPERTIES
    EXPORT_NAME "kbibtexnetworking"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

target_include_directories(kbibtexnetworking
    INTERFACE
        $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KBibTeX/networking>
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(kbibtexnetworking
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Network
        Qt${QT_MAJOR_VERSION}::Widgets
        KBibTeX::Data
    PRIVATE
        Poppler::Qt${QT_MAJOR_VERSION}
        Qt${QT_MAJOR_VERSION}::DBus
        Qt${QT_MAJOR_VERSION}::NetworkAuth
        KF${QT_MAJOR_VERSION}::ConfigCore
        KF${QT_MAJOR_VERSION}::WidgetsAddons
        KF${QT_MAJOR_VERSION}::I18n
        KF${QT_MAJOR_VERSION}::KIOCore
        KF${QT_MAJOR_VERSION}::KIOFileWidgets
        KBibTeX::Config
        KBibTeX::Global
        KBibTeX::IO
)

if(Qt${QT_MAJOR_VERSION}WebEngineWidgets_FOUND)
    # See also above for a similar if-clause
    target_link_libraries(kbibtexnetworking
        PRIVATE
            Qt${QT_MAJOR_VERSION}::WebEngineWidgets
    )
endif()

set(onlinesearchgeneratorstems "arxiv" "ieeexplore" "pubmed" "springerlink" "zbmath")
set(onlinesearchdependencies)
foreach(stem ${onlinesearchgeneratorstems})
    add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/onlinesearch/onlinesearch${stem}-parser.generated.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/onlinesearch/onlinesearch-parser-generator.py ${CMAKE_CURRENT_SOURCE_DIR}/onlinesearch/onlinesearch${stem}-parser.in.cpp
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/onlinesearch/onlinesearch-parser-generator.py ${CMAKE_CURRENT_SOURCE_DIR}/onlinesearch/onlinesearch${stem}-parser.in.cpp > ${CMAKE_CURRENT_BINARY_DIR}/onlinesearch/onlinesearch${stem}-parser.generated.cpp
    VERBATIM
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/onlinesearch/onlinesearch${stem}-parser.generated.cpp
        PROPERTIES
        GENERATED 1
        HEADER_FILE_ONLY 1
        SKIP_AUTOMOC ON
        SKIP_AUTOUIC ON
        SKIP_AUTOGEN ON
    )
    list(APPEND onlinesearchdependencies ${CMAKE_CURRENT_BINARY_DIR}/onlinesearch/onlinesearch${stem}-parser.generated.cpp)
endforeach()
add_custom_target(
    parserincludes
    DEPENDS
        ${onlinesearchdependencies}
)
add_dependencies(kbibtexnetworking
    parserincludes
)

install(
    TARGETS kbibtexnetworking
    EXPORT kbibtexnetworking-targets
    LIBRARY NAMELINK_SKIP
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

set_target_properties(kbibtexnetworking PROPERTIES
    EXPORT_NAME "Networking"
)

ecm_generate_headers(kbibtexnetworking_HEADERS
    HEADER_NAMES
        AssociatedFiles
        FindPDF
        FavIconLocator
        InternalNetworkAccessManager
        UrlChecker
        onlinesearch/OnlineSearchAbstract
        onlinesearch/OnlineSearchGeneral
        onlinesearch/OnlineSearchAcmPortal
        onlinesearch/OnlineSearchArXiv
        onlinesearch/OnlineSearchBibsonomy
        onlinesearch/OnlineSearchBioRxiv
        onlinesearch/OnlineSearchCERNDS
        onlinesearch/OnlineSearchDOI
        onlinesearch/OnlineSearchGoogleScholar
        onlinesearch/OnlineSearchIDEASRePEc
        onlinesearch/OnlineSearchIEEEXplore
        onlinesearch/OnlineSearchIngentaConnect
        onlinesearch/OnlineSearchInspireHep
        onlinesearch/OnlineSearchJStor
        onlinesearch/OnlineSearchMathSciNet
        onlinesearch/OnlineSearchMRLookup
        onlinesearch/OnlineSearchPubMed
        onlinesearch/OnlineSearchScienceDirect
        onlinesearch/OnlineSearchSemanticScholar
        onlinesearch/OnlineSearchSimpleBibTeXDownload
        onlinesearch/OnlineSearchSOANASAADS
        onlinesearch/OnlineSearchSpringerLink
        onlinesearch/OnlineSearchUnpaywall
        onlinesearch/OnlineSearchZbMath
        zotero/API
        zotero/Collection
        zotero/CollectionModel
        zotero/Groups
        zotero/Items
        zotero/OAuthWizard
        zotero/TagModel
        zotero/Tags
    REQUIRED_HEADERS kbibtexnetworking_HEADERS
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexnetworking_export.h
    ${kbibtexnetworking_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX/networking
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXNetworking-configVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/KBibTeXNetworking-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXNetworking-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXNetworking-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXNetworking-configVersion.cmake
    DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)
