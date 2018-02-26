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
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/global
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/config
    ${CMAKE_SOURCE_DIR}/src/gui/bibtex
    ${CMAKE_SOURCE_DIR}/src/gui/element
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_SOURCE_DIR}/src/networking/onlinesearch
    ${CMAKE_SOURCE_DIR}/src/processing
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

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtextest kbibtextest_SRCS)
    enable_unity_build(kbibtexfilestest kbibtexfilestest_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

# Creates kbibtex-git-info.h containing information about the source code's Git revision
# (if source directory is a Git clone)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/kbibtex-git-info.h
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getgit.cmake
)
set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtex-git-info.h
    PROPERTIES
    GENERATED 1
    HEADER_FILE_ONLY 1
    SKIP_AUTOMOC ON
    SKIP_AUTOUIC ON
    SKIP_AUTOGEN ON
)

add_executable(
    kbibtextest
    ${kbibtextest_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtex-git-info.h
)

add_executable(
    kbibtexfilestest
    ${kbibtexfilestest_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtex-git-info.h
)

target_link_libraries( kbibtextest
    Qt5::Core
    KF5::KIOCore
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexproc
    kbibtexgui
    kbibtexnetworking
)

target_link_libraries( kbibtexfilestest
    Qt5::Test
    kbibtexdata
    kbibtexio
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
