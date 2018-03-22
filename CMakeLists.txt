if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif(POLICY CMP0048)

project(
    kbibtex
    VERSION "0.8.50"
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
include(ECMSetupVersion)
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

if (MSVC)
    MESSAGE( STATUS "Disabling building tests when using Microsoft Visual Studio C++ compiler" )
    # Note to any developer: Try to enable building tests and see which issues you may encounter.
    # Examples may include: (1) char* texts which exceed the size limit supported by MSVC which
    # is about 2^16 bytes and (2) characters in strings written in \uXXXX notation not supported
    # in 1252 encoding as assumed by MSVC for C++ source files.
    SET(BUILD_TESTING OFF)
endif (MSVC)

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

    set(
        TESTSET_DIRECTORY ""
        CACHE PATH
        "Directory where the local checkout of Git repository 'kbibtex-testset' is located"
    )
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

ecm_setup_version(
    PROJECT
    VARIABLE_PREFIX KBIBTEX
    SOVERSION ${KBIBTEX_VERSION_MAJOR}
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kbibtex-version.h"
)

if((${KBIBTEX_VERSION_PATCH} GREATER 50) OR (${KBIBTEX_VERSION_PATCH} EQUAL 50))
    # If the version number indicates a pre-release version such as
    # '0.7.90', i.e. a beta version for the major release 0.8,
    # increment release version from 0.7 to 0.8
    math(EXPR KBIBTEX_RELEASE_VERSION_MINOR "${KBIBTEX_VERSION_MINOR} + 1")
    set(
        KBIBTEX_RELEASE_VERSION ${KBIBTEX_VERSION_MAJOR}.${KBIBTEX_RELEASE_VERSION_MINOR}
    )
else((${KBIBTEX_VERSION_PATCH} GREATER 50) OR (${KBIBTEX_VERSION_PATCH} EQUAL 50))
    set(
        KBIBTEX_RELEASE_VERSION ${KBIBTEX_VERSION_MAJOR}.${KBIBTEX_VERSION_MINOR}
    )
endif((${KBIBTEX_VERSION_PATCH} GREATER 50) OR (${KBIBTEX_VERSION_PATCH} EQUAL 50))

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
find_package(
    QtOAuth
)
if (Qca-qt5_FOUND AND QTOAUTH_FOUND)
    message(STATUS "Found Qca-qt5 " ${Qca-qt5_VERSION} " and QtOAuth")
    add_definitions(-DHAVE_ZOTERO=1)
    set(BUILD_ZOTERO ON)
else(Qca-qt5_FOUND AND QTOAUTH_FOUND)
    message(STATUS "No Qca-qt5 or no QtOAuth, disabling certain functionality")
    set(BUILD_ZOTERO OFF)
endif(Qca-qt5_FOUND AND QTOAUTH_FOUND)

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
