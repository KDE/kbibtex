project(
    kbibtex
)

cmake_minimum_required(
    VERSION
    3.2.0
    FATAL_ERROR
)

set(
    QT_MIN_VERSION "5.6.0"
)
set(
    KF5_MIN_VERSION "5.26.0"
)

find_package(ECM 5.19 REQUIRED NO_MODULE)

add_definitions(
    -DTRANSLATION_DOMAIN="kbibtex"
    -DHAVE_QTWIDGETS
    -DHAVE_KF5
    -DHAVE_ICU
)

set(
    CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake/modules
    ${ECM_MODULE_PATH}
    ${ECM_KDE_MODULE_DIR}
    ${CMAKE_MODULE_PATH}
)


include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(GenerateExportHeader)

find_package(
    Qt5 ${QT_MIN_VERSION}
    CONFIG
    REQUIRED
    Core
    Widgets
    Network
    XmlPatterns
)

if(
    BUILD_TESTING
)
    find_package(
        Qt5Test ${QT_MIN_VERSION}
        CONFIG
        REQUIRED
    )

    if (WRITE_RAWDATAFILE)
        add_definitions(-DWRITE_RAWDATAFILE)
    endif(WRITE_RAWDATAFILE)
endif(
    BUILD_TESTING
)

find_package(
    Qt5WebEngineWidgets ${QT_MIN_VERSION}
    QUIET
    CONFIG
)

find_package(
    Qt5WebKitWidgets ${QT_MIN_VERSION}
    QUIET
    CONFIG
)

find_package(
    KF5 ${KF5_MIN_VERSION}
    REQUIRED
    I18n
    XmlGui
    KIO
    IconThemes
    ItemViews
    Completion
    Parts
    CoreAddons
    Service
    Wallet
    Crash
)

find_package(KF5DocTools)

# Version number for the exported headers / libs
set(
    VERSION_BRANCH "0.8"
)
set(
    REVISION_VERSION "0"
)
set(
    LIB_VERSION ${VERSION_BRANCH}.${REVISION_VERSION}
)
set(
    LIB_SOVERSION ${VERSION_BRANCH}
)

option(
    UNITY_BUILD
    "Compile multiple C++ files in one big, merged file (\"Unity build\")\nSee also http://t-fischer.dreamwidth.org/3054.html"
)
if(UNITY_BUILD)
    message(STATUS "Unity build enabled")
else(UNITY_BUILD)
    message(STATUS "Unity build disabled (default), use option UNITY_BUILD to enable it")
endif(UNITY_BUILD)

# # FIXME may have to be cleaned up a little bit
# # Contributed by Jeremy Cribb <cribb.jeremy@gmail.com>
# if(
#     APPLE
# )
#     find_library(
#         SYS_CONFIG_LIBRARY
#         SystemConfiguration
#     )
#     mark_as_advanced(
#         SYS_CONFIG_LIBRARY
#     )
#     # SET(TARGET_EXTERNAL_LIBRARIES iconv ${SYS_CONFIG_LIBRARY})
#     set(
#         ICONV_INCLUDE_DIR
#         "/opt/local/include"
#     )
#     set(
#         ICONV_LIBRARIES
#         "/opt/local/lib/libiconv.dylib"
#     )
#     set(
#         LIBXSLT_LIBRARIES
#         "/opt/local/lib/libxslt.dylib"
#     )
# endif(
#     APPLE
# )

find_package(
    Poppler
    REQUIRED
    COMPONENTS
    Qt5
)

find_package(
    ICU
    REQUIRED
    COMPONENTS uc i18n
)
if(
    ICU_FOUND
)
    message(
        STATUS
        "Found ICU " ${ICU_VERSION}
    )
    include_directories(${ICU_INCLUDE_DIRS})
endif(
    ICU_FOUND
)

find_package(
    Qca-qt5 "2.1.0"
)
if (Qca-qt5_FOUND)
    message(STATUS "Found Qca-qt5 " ${Qca-qt5_VERSION})
    add_definitions(-DHAVE_ZOTERO=1)
    set(BUILD_ZOTERO ON)
else(Qca-qt5_FOUND)
    message(STATUS "No Qca-qt5, disabling certain functionality")
    set(BUILD_ZOTERO OFF)
endif(Qca-qt5_FOUND)

# Make an educated guess where to find headers for QCA-Qt5
# TODO isn't there an "official" way to determine this directory?
find_path(
    QTCRYPTO_INCLUDE_DIR qca.h
    HINTS ENV ${Qt5Core_INCLUDE_DIRS} ENV ${QT_INCLUDES}
    PATH_SUFFIXES QtCrypto
)


add_subdirectory(
    config
)
add_subdirectory(
    src
)
add_subdirectory(
    xslt
)
add_subdirectory(
    mime
)
if(KF5DocTools_FOUND)
    add_subdirectory(doc)
endif()
# macro_optional_add_subdirectory(
#     po
# )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
