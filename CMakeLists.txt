# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2008-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2013-2021 Pino Toscano <pino@kde.org>
# SPDX-FileContributor: 2014-2021 Yuri Chornoivan <yurchor@ukr.net>
# SPDX-FileContributor: 2015-2021 David Faure <faure@kde.org>
# SPDX-FileContributor: 2016-2021 Andreas Sturmlechner <andreas.sturmlechner@gmail.com>

cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

project(
    kbibtex
    VERSION 0.10.50
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(KF_MIN_VERSION "5.86")
set(QT_REQUIRED_VERSION "5.15.2")
set(QT_MAJOR_VERSION "5")
if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
endif()

set(KDE_COMPILERSETTINGS_LEVEL ${KF_MIN_VERSION})
find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

set(
    CMAKE_MODULE_PATH
    ${ECM_MODULE_PATH}
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


option(
    BUILD_APP_DESKTOP
    "Build the desktop app which makes use of widgets"
    ON
)
option(
    BUILD_APP_COMMAND_LINE
    "Build the command line program with minimal dependencies only"
    ON
)
option(
    BUILD_KPART
    "Build KBibTeX's KPart (will be set ON automatically if desktop app is built)"
    OFF
)
if(BUILD_APP_DESKTOP)
    set(BUILD_KPART ON)
endif()

find_package(
    Python
    COMPONENTS
      Interpreter
    REQUIRED
)

find_package(
    Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION}
    CONFIG
    COMPONENTS
    Core
)

if (QT_MAJOR_VERSION STREQUAL "6")
    # Should become necessary once migrated from QTextCodec to QStringConverter
    find_package(
        Qt6 ${QT_REQUIRED_VERSION}
        CONFIG
        COMPONENTS
        Core5Compat
    )
endif()

if (QT_MAJOR_VERSION STREQUAL "5")
    # Migrate away from QXmlQuery
    find_package(
        Qt5 ${QT_REQUIRED_VERSION}
        CONFIG
        COMPONENTS
        XmlPatterns
    )
endif()

if(BUILD_KPART OR BUILD_TESTING)
    find_package(
        Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION}
        CONFIG
        COMPONENTS
        Gui
        Widgets
        Network
        Concurrent
        NetworkAuth
        OPTIONAL_COMPONENTS
        WebEngineWidgets
        WebKitWidgets
        Test
    )
    if (Qt${QT_MAJOR_VERSION}Widgets_FOUND)
        add_compile_definitions(HAVE_QTWIDGETS)
    endif()
endif()

if(Qt${QT_MAJOR_VERSION}WebEngineWidgets_FOUND)
    add_compile_definitions(HAVE_WEBENGINEWIDGETS)
endif()

if (Qt${QT_MAJOR_VERSION}XmlPatterns_FOUND)
    add_compile_definitions(HAVE_QTXMLPATTERNS)
endif()

find_package(
    KF${QT_MAJOR_VERSION} ${KF_MIN_VERSION}
    MODULE
    REQUIRED
    Config
)
if(BUILD_KPART OR BUILD_TESTING)
    find_package(
        KF${QT_MAJOR_VERSION} ${KF_MIN_VERSION}
        MODULE
        REQUIRED
        I18n
        KIO
        IconThemes
        CoreAddons
        Wallet
        Crash
        TextEditor
        TextWidgets
        COMPONENTS
        XmlGui
        Parts
    )
endif()
if (KF5_FOUND)
    add_compile_definitions(HAVE_KF5 HAVE_KF)
endif()
if (KF6_FOUND)
    add_compile_definitions(HAVE_KF6 HAVE_KF)
endif()
if (KF${QT_MAJOR_VERSION}I18n_FOUND)
    add_compile_definitions(HAVE_KFI18N)
endif()

find_package(
    Poppler
    MODULE
    REQUIRED
    Qt${QT_MAJOR_VERSION}
)
if (PKG_Poppler_Qt${QT_MAJOR_VERSION}_FOUND)
    add_compile_definitions(HAVE_POPPLERQT${QT_MAJOR_VERSION} HAVE_POPPLERQT)
endif()

find_package(
    ICU
    MODULE
    OPTIONAL_COMPONENTS
    uc i18n
)

if(ICU_FOUND)
    add_compile_definitions(HAVE_ICU)
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
if(NOT BUILD_TESTING AND Qt${QT_MAJOR_VERSION}Test_FOUND)
    message(STATUS
        "Testing is disabled, but can be enabled as the Qt::Test library is available"
    )
endif()
if(BUILD_TESTING AND NOT Qt${QT_MAJOR_VERSION}Test_FOUND)
    message(STATUS
        "Disabling building tests as Qt::Test library is not available"
    )
    set(BUILD_TESTING OFF)
endif()

if(BUILD_TESTING)
    add_compile_definitions(BUILD_TESTING)
    if (WRITE_RAWDATAFILE)
        add_compile_definitions(WRITE_RAWDATAFILE)
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


option(
    BUILD_SCIHUB
    "Support fetching PDF files from SciHub"
    ON
)
if(BUILD_SCIHUB)
    add_compile_definitions(HAVE_SCIHUB)
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
if(KF${QT_MAJOR_VERSION}DocTools_FOUND)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()

if(KF${QT_MAJOR_VERSION}I18n_FOUND)
    ki18n_install(po)
endif()

if (ECM_VERSION VERSION_GREATER_EQUAL "5.59.0")
    install(FILES kbibtex.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES kbibtex.categories DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
