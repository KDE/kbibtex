cmake_minimum_required(VERSION 3.7.2)

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif(POLICY CMP0048)

project(
    kbibtex
    VERSION "0.9.50"
)

set(
    QT_MIN_VERSION "5.9.0"
)
set(
    KF5_MIN_VERSION "5.51.0"
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
    ${ECM_MODULE_PATH}
    ${ECM_KDE_MODULE_DIR}
    ${CMAKE_MODULE_PATH}
)


include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMGenerateHeaders)
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
    Concurrent
    NetworkAuth
    OPTIONAL_COMPONENTS
    WebEngineWidgets
    WebKitWidgets
    Test
)


if (MSVC)
    MESSAGE( STATUS "Disabling building tests when using Microsoft Visual Studio C++ compiler" )
    # Note to any developer: Try to enable building tests and see which issues you may encounter.
    # Examples may include: (1) char* texts which exceed the size limit supported by MSVC which
    # is about 2^16 bytes and (2) characters in strings written in \uXXXX notation not supported
    # in 1252 encoding as assumed by MSVC for C++ source files.
    SET(BUILD_TESTING OFF)
endif (MSVC)
if(BUILD_TESTING AND NOT Qt5Test_FOUND)
    message(STATUS
        "Disabling building tests as Qt5::Test library is not available"
    )
    set(BUILD_TESTING OFF)
endif()

if(
    BUILD_TESTING
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

find_package(KF5TextEditor NO_MODULE)

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
