set(
    kbibtexnetworking_LIB_SRCS
    onlinesearch/onlinesearchabstract.cpp
    onlinesearch/onlinesearchbibsonomy.cpp
    onlinesearch/onlinesearcharxiv.cpp
    onlinesearch/onlinesearchsciencedirect.cpp
    onlinesearch/onlinesearchgooglescholar.cpp
    onlinesearch/onlinesearchieeexplore.cpp
    onlinesearch/onlinesearchpubmed.cpp
    onlinesearch/onlinesearchacmportal.cpp
    onlinesearch/onlinesearchspringerlink.cpp
    onlinesearch/onlinesearchjstor.cpp
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
    internalnetworkaccessmanager.cpp
    logging_networking.cpp
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexnetworking kbibtexnetworking_LIB_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexnetworking
    SHARED
    ${kbibtexnetworking_LIB_SRCS}
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
)

target_link_libraries(kbibtexnetworking
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::Network
        Qt5::Widgets
        # TODO refactor code to not expose config interface
        KF5::ConfigCore
        KF5::ConfigGui
        KBibTeX::Data
    PRIVATE
        Poppler::Qt5
        Qt5::DBus
        Qt5::NetworkAuth
        KF5::WidgetsAddons
        KF5::I18n
        KF5::KIOCore
        KF5::KIOFileWidgets
        KBibTeX::Config
        KBibTeX::Global
        KBibTeX::IO
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
        InternalNetworkAccessManager
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
