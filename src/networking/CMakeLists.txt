# Network library

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
    onlinesearch/onlinesearchisbndb.cpp
    onlinesearch/onlinesearchideasrepec.cpp
    onlinesearch/onlinesearchdoi.cpp
    onlinesearch/onlinesearchbiorxiv.cpp
    zotero/collectionmodel.cpp
    zotero/collection.cpp
    zotero/api.cpp
    zotero/groups.cpp
    zotero/tags.cpp
    zotero/tagmodel.cpp
    zotero/items.cpp
    zotero/oauthwizard.cpp
    associatedfiles.cpp
    findpdf.cpp
    internalnetworkaccessmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/global/kbibtex.cpp
    logging_networking.cpp
    # QOAuth for Qt5 from Git
    ../3rdparty/qoauth/src/interface.cpp
)

set(
    kbibtexnetworking_HDRS
    onlinesearch/onlinesearchgeneral.h
    onlinesearch/onlinesearchsciencedirect.h
    onlinesearch/onlinesearchabstract.h
    onlinesearch/onlinesearchacmportal.h
    onlinesearch/onlinesearchbibsonomy.h
    onlinesearch/onlinesearchgooglescholar.h
    onlinesearch/onlinesearchspringerlink.h
    onlinesearch/onlinesearchjstor.h
    onlinesearch/onlinesearchieeexplore.h
    onlinesearch/onlinesearcharxiv.h
    onlinesearch/onlinesearchpubmed.h
    onlinesearch/onlinesearchingentaconnect.h
    onlinesearch/onlinesearchsimplebibtexdownload.h
    onlinesearch/onlinesearchsoanasaads.h
    onlinesearch/onlinesearchmathscinet.h
    onlinesearch/onlinesearchmrlookup.h
    onlinesearch/onlinesearchinspirehep.h
    onlinesearch/onlinesearchcernds.h
    onlinesearch/onlinesearchisbndb.h
    onlinesearch/onlinesearchideasrepec.h
    onlinesearch/onlinesearchdoi.h
    onlinesearch/onlinesearchbiorxiv.h
    zotero/collectionmodel.h
    zotero/collection.h
    zotero/groups.h
    zotero/tags.h
    zotero/tagmodel.h
    zotero/items.h
    zotero/api.h
    zotero/oauthwizard.h
    associatedfiles.h
    findpdf.h
    internalnetworkaccessmanager.h
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexnetworking kbibtexnetworking_LIB_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/3rdparty/qoauth/include
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_BINARY_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/networking/onlinesearch
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_SOURCE_DIR}/src/global
)

add_library(
    kbibtexnetworking
    SHARED
    ${kbibtexnetworking_LIB_SRCS}
)

target_link_libraries(
    kbibtexnetworking
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    KF5::I18n
    KF5::XmlGui
    KF5::Completion
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::KIONTLM
    KF5::Wallet
    kbibtexconfig
    kbibtexdata
    kbibtexio
    qca-qt5
    ${Poppler_Qt5_LIBRARY}
)

set_target_properties(
    kbibtexnetworking
    PROPERTIES
    EXPORT_NAME "kbibtexnetworking"
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexnetworking
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexnetworking )

# install(
#     FILES
#     ${kbibtexnetworking_HDRS}
#     ${CMAKE_BINARY_DIR}/src/networking/kbibtexnetworking_export.h
#     DESTINATION
#     ${INCLUDE_INSTALL_DIR}/kbibtex/networking
#     COMPONENT
#     development
# )
