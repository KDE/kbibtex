# KBibTeX test program

project(
    test
)

add_definitions(
    -DTESTSET_DIRECTORY="\\\"${CMAKE_SOURCE_DIR}/testset/\\\""
)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
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
)

# # debug area for KBibTeX's test program
# add_definitions(
#     -DKDE_DEFAULT_DEBUG_AREA=101023
# )

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtextest kbibtextest_SRCS)
    enable_unity_build(kbibtexfilestest kbibtexfilestest_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

add_executable(
    kbibtextest
    ${kbibtextest_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# add_unit_test(
#     kbibtexfilestest
#     ${kbibtexfilestest_SRCS}
#     ${CMAKE_CURRENT_BINARY_DIR}/version.h
# )

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

# target_link_libraries( kbibtexfilestest
#     Qt5::Test
#     kbibtexdata
#     kbibtexio
# )

# version.h is a generated file
set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    PROPERTIES
    GENERATED
    TRUE
    HEADER_FILE_ONLY
    TRUE
)

add_dependencies( kbibtextest
    GITrevisionTest
)

# add_dependencies( kbibtexfilestest
#     GITrevisionTest
# )

# creates version.h using cmake script
add_custom_target(
    GITrevisionTest
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getgit.cmake
    COMMENT
    "Determine Git revision"
)
