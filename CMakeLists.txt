cmake_minimum_required(VERSION 3.7.2)

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif(POLICY CMP0048)

project(
    kbibtex
    VERSION 0.10.50
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 11)
set(QT_MIN_VERSION 5.9.0)
# Somewhat arbitrary chosen version number ...
set(KF5_MIN_VERSION 5.51)
find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)

set(
    CMAKE_MODULE_PATH
    ${ECM_MODULE_PATH}
    ${ECM_KDE_MODULE_DIR}
    ${CMAKE_MODULE_PATH}
)

set(KDE_INSTALL_DIRS_NO_DEPRECATED TRUE)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMGenerateHeaders)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddAppIcon)
include(GenerateExportHeader)
include(ECMQtDeclareLoggingCategory)

ecm_setup_version(
    PROJECT
    VARIABLE_PREFIX KBIBTEX
    SOVERSION ${KBIBTEX_VERSION_MAJOR}
    VERSION_HEADER "${CMAKE_BINARY_DIR}/kbibtex-version.h"
    PACKAGE_VERSION_FILE "${CMAKE_BINARY_DIR}/KBibTeXConfigVersion.cmake"
)

install(
    FILES ${CMAKE_BINARY_DIR}/kbibtex-version.h
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX
    # FIXME is "Devel" standard?
    COMPONENT Devel
)


if("${KBIBTEX_VERSION_PATCH}" STREQUAL "")
    # Patch level is not set for version numbers like "0.9",
    # so set the patch level manually to 0
    set(KBIBTEX_VERSION_PATCH 0)
endif()

if((${KBIBTEX_VERSION_PATCH} GREATER 50) OR (${KBIBTEX_VERSION_PATCH} EQUAL 50))
    # If the version number indicates a pre-release version such as
    # '0.7.90', i.e. a beta version for the major release 0.8,
    # increment release version from 0.7 to 0.8
    math(EXPR KBIBTEX_RELEASE_VERSION_MINOR "${KBIBTEX_VERSION_MINOR} + 1")
    set(
        KBIBTEX_RELEASE_VERSION ${KBIBTEX_VERSION_MAJOR}.${KBIBTEX_RELEASE_VERSION_MINOR}
    )
else()
    set(
        KBIBTEX_RELEASE_VERSION ${KBIBTEX_VERSION_MAJOR}.${KBIBTEX_VERSION_MINOR}
    )
endif()

option(
    UNITY_BUILD
    "Compile multiple C++ files in one big, merged file (\"Unity build\")\nSee also https://t-fischer.dreamwidth.org/3054.html"
)
if(UNITY_BUILD)
    message(STATUS "Unity build enabled")
else(UNITY_BUILD)
    message(STATUS "Unity build disabled (default), use option UNITY_BUILD to enable it")
endif(UNITY_BUILD)

find_package(
    Qt5 ${QT_MIN_VERSION}
    CONFIG
    COMPONENTS
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
if (Qt5Widgets_FOUND)
    add_definitions(-DHAVE_QTWIDGETS)
endif()
if(Qt5WebEngineWidgets_FOUND)
    # Once CMake 3.12.x is minimum requirement, use 'add_compile_definitions'
    add_definitions(
        -DHAVE_WEBENGINEWIDGETS
    )
endif()

find_package(
    KF5 ${KF5_MIN_VERSION}
    MODULE
    REQUIRED
    I18n
    XmlGui
    KIO
    IconThemes
    Parts
    CoreAddons
    Service
    Wallet
    Crash
    TextEditor
    OPTIONAL_COMPONENTS
    DocTools
)
if (KF5_FOUND)
    add_definitions(-DHAVE_KF5)
endif()

find_package(
    Poppler
    MODULE
    REQUIRED
    Qt5
)

find_package(
    ICU
    MODULE
    OPTIONAL_COMPONENTS
    uc i18n
)
if(ICU_FOUND)
    add_definitions(-DHAVE_ICU)
endif()

option(
    BUILD_TESTING
    "Build automated and interactive tests"
    OFF
)
if (MSVC)
    MESSAGE( STATUS "Disabling building tests when using Microsoft Visual Studio C++ compiler" )
    # Note to any developer: Try to enable building tests and see which issues you may encounter.
    # Examples may include: (1) char* texts which exceed the size limit supported by MSVC which
    # is about 2^16 bytes and (2) characters in strings written in \uXXXX notation not supported
    # in 1252 encoding as assumed by MSVC for C++ source files.
    set(BUILD_TESTING OFF)
endif()
if(NOT BUILD_TESTING AND Qt5Test_FOUND)
    message(STATUS
        "Testing is disabled, but can be enabled as the Qt5::Test library is available"
    )
endif()
if(BUILD_TESTING AND NOT Qt5Test_FOUND)
    message(STATUS
        "Disabling building tests as Qt5::Test library is not available"
    )
    set(BUILD_TESTING OFF)
endif()

if(BUILD_TESTING)
    if (WRITE_RAWDATAFILE)
        add_definitions(-DWRITE_RAWDATAFILE)
    endif(WRITE_RAWDATAFILE)

    set(
        TESTSET_DIRECTORY ""
        CACHE PATH
        "Directory where the local checkout of Git repository 'kbibtex-testset' is located"
    )
endif()

if(TESTSET_DIRECTORY AND ( NOT EXISTS "${TESTSET_DIRECTORY}/bib/minix.bib" OR NOT EXISTS "${TESTSET_DIRECTORY}/bib/backslash.bib" ))
    message("Variable TESTSET_DIRECTORY is set to '${TESTSET_DIRECTORY}' but various BibTeX files were not found. Unsetting TESTSET_DIRECTORY.")
    unset(TESTSET_DIRECTORY)
    unset(TESTSET_DIRECTORY CACHE)
endif()


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

if (ECM_VERSION VERSION_GREATER_EQUAL "5.59.0")
    install(FILES kbibtex.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES kbibtex.categories DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
