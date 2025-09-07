# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2012-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2018 David Faure <faure@kde.org>
# SPDX-FileContributor: 2014-2016 Pino Toscano <pino@kde.org>

include(AddFileDependencies)
include(ECMMarkAsTest)

configure_file(test-config.h.in test-config.h @ONLY)

set(
    kbibtextest_SRCS
    main.cpp
    kbibtextest.cpp
)

ecm_qt_declare_logging_category(kbibtextest_SRCS
    HEADER logging_test.h
    IDENTIFIER LOG_KBIBTEX_TEST
    CATEGORY_NAME kbibtex.test
)


set(
    kbibtexnetworkingtest_SRCS
    kbibtexnetworkingtest.cpp
)

set(
    kbibtexiotest_SRCS
    kbibtexiotest.cpp
)

ecm_qt_declare_logging_category(kbibtexiotest_SRCS
    HEADER logging_test.h
    IDENTIFIER LOG_KBIBTEX_TEST
    CATEGORY_NAME kbibtex.test
)

set(
    kbibtexdatatest_SRCS
    kbibtexdatatest.cpp
)

set(
    kbibtexguitest_SRCS
    kbibtexguitest.cpp
)

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtextest kbibtextest_SRCS)
    enable_unity_build(kbibtexnetworkingtest kbibtexnetworkingtest_SRCS)
    enable_unity_build(kbibtexiotest kbibtexiotest_SRCS)
    enable_unity_build(kbibtexdatatest kbibtexdatatest_SRCS)
    enable_unity_build(kbibtexguitest kbibtexdatatest_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

add_executable(
    kbibtextest
    ${kbibtextest_SRCS}
)

add_dependencies(kbibtextest
    generate-kbibtex-git-info
)

add_executable(
    kbibtexnetworkingtest
    ${kbibtexnetworkingtest_SRCS}
)

add_dependencies(kbibtexnetworkingtest
    generate-kbibtex-git-info
)

add_executable(
    kbibtexiotest
    ${kbibtexiotest_SRCS}
)

add_dependencies(kbibtexiotest
    generate-kbibtex-git-info
)

add_executable(
    kbibtexdatatest
    ${kbibtexdatatest_SRCS}
)

add_dependencies(kbibtexdatatest
    generate-kbibtex-git-info
)

add_executable(
    kbibtexguitest
    ${kbibtexguitest_SRCS}
)

add_dependencies(kbibtexguitest
    generate-kbibtex-git-info
)

target_link_libraries(kbibtextest
    Qt${QT_MAJOR_VERSION}::Core
    KF${QT_MAJOR_VERSION}::KIOCore
    KF${QT_MAJOR_VERSION}::I18n
    KBibTeX::Config
    KBibTeX::Data
    KBibTeX::IO
    KBibTeX::Processing
    KBibTeX::GUI
    KBibTeX::Networking
)

target_include_directories(kbibtextest
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kbibtexnetworkingtest
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Test
        KBibTeX::Networking
)

target_include_directories(kbibtexnetworkingtest
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kbibtexiotest
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Test
        KF${QT_MAJOR_VERSION}::I18n
        KF${QT_MAJOR_VERSION}::ConfigCore
        KBibTeX::IO
        ICU::uc
        ICU::i18n
)

# FIXME get rid of Qt6Core5Compat
if(Qt6Core5Compat_FOUND)
    target_link_libraries(kbibtexiotest
        PRIVATE
            Qt6::Core5Compat
    )
endif()

target_include_directories(kbibtexiotest
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kbibtexdatatest
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Test
        KBibTeX::Data
        KBibTeX::IO
)

target_include_directories(kbibtexdatatest
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kbibtexguitest
    PRIVATE
        Qt${QT_MAJOR_VERSION}::Test
        KBibTeX::Global
        KBibTeX::Data
        KBibTeX::GUI
)

target_include_directories(kbibtexguitest
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

ecm_mark_as_test(
    kbibtexnetworkingtest
    kbibtexiotest
    kbibtexdatatest
    kbibtexguitest
)

add_test(
    NAME
    kbibtexnetworkingtest
    COMMAND
    kbibtexnetworkingtest
)

add_test(
    NAME
    kbibtexiotest
    COMMAND
    kbibtexiotest
)

add_test(
    NAME
    kbibtexdatatest
    COMMAND
    kbibtexdatatest
)

add_test(
    NAME
    kbibtexguitest
    COMMAND
    kbibtexguitest
)

if(TESTSET_DIRECTORY)
    set(
        kbibtexfilestest_SRCS
        kbibtexfilestest.cpp
        kbibtexfilestest-rawdata.h
    )
    ecm_qt_declare_logging_category(kbibtexfilestest_SRCS
        HEADER logging_test.h
        IDENTIFIER LOG_KBIBTEX_TEST
        CATEGORY_NAME kbibtex.test
    )
    if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
        enable_unity_build(kbibtexfilestest kbibtexfilestest_SRCS)
    endif()
    add_executable(
        kbibtexfilestest
        ${kbibtexfilestest_SRCS}
    )
    add_dependencies(kbibtexfilestest
        generate-kbibtex-git-info
    )
    target_link_libraries(kbibtexfilestest
        PRIVATE
            Qt${QT_MAJOR_VERSION}::Test
            KBibTeX::Data
            KBibTeX::IO
    )
    target_include_directories(kbibtexfilestest
        PRIVATE
            ${CMAKE_BINARY_DIR}
    )
    ecm_mark_as_test(
        kbibtexfilestest
    )
    add_test(
        NAME
        kbibtexfilestest
        COMMAND
        kbibtexfilestest
    )
endif()
