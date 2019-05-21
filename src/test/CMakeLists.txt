# KBibTeX test program

project(
    test
)

include(
    AddFileDependencies
)
include(
    ECMMarkAsTest
)

configure_file(test-config.h.in test-config.h @ONLY)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(
    kbibtextest_SRCS
    main.cpp
    kbibtextest.cpp
    logging_test.cpp
)

set(
    kbibtexfilestest_SRCS
    kbibtexfilestest.cpp
    kbibtexfilestest-rawdata.h
)

set(
    kbibtexnetworkingtest_SRCS
    kbibtexnetworkingtest.cpp
)

set(
    kbibtexiotest_SRCS
    kbibtexiotest.cpp
)

set(
    kbibtexdatatest_SRCS
    kbibtexdatatest.cpp
)

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtextest kbibtextest_SRCS)
    enable_unity_build(kbibtexfilestest kbibtexfilestest_SRCS)
    enable_unity_build(kbibtexnetworkingtest kbibtexnetworkingtest_SRCS)
    enable_unity_build(kbibtexiotest kbibtexiotest_SRCS)
    enable_unity_build(kbibtexdatatest kbibtexdatatest_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

add_executable(
    kbibtextest
    ${kbibtextest_SRCS}
)

add_dependencies(kbibtextest
    generate-kbibtex-git-info
)

add_executable(
    kbibtexfilestest
    ${kbibtexfilestest_SRCS}
)

add_dependencies(kbibtexfilestest
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

target_link_libraries( kbibtextest
    Qt5::Core
    KF5::KIOCore
    KF5::I18n
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexprocessing
    kbibtexgui
    kbibtexnetworking
)

target_link_libraries( kbibtexfilestest
    Qt5::Test
    kbibtexdata
    kbibtexio
)

target_link_libraries( kbibtexnetworkingtest
    Qt5::Test
    kbibtexnetworking
)

target_link_libraries( kbibtexiotest
    Qt5::Test
    kbibtexio
)

target_link_libraries( kbibtexdatatest
    Qt5::Test
    kbibtexdata
)

ecm_mark_as_test(
    kbibtexfilestest
    kbibtexnetworkingtest
    kbibtexiotest
    kbibtexdatatest
)

add_test(
    NAME
    kbibtexfilestest
    COMMAND
    kbibtexfilestest
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
