project(
    test
)

include(
    AddFileDependencies
)

add_definitions(
    -DTESTSET_DIRECTORY=\\"${CMAKE_SOURCE_DIR}/testset/\\"
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
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(
    kbibtextest_SRCS
    main.cpp
    kbibtextest.cpp
)

# debug area for KBibTeX's test program
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101023
)

kde4_add_app_icon(
    kbibtextest_SRCS
    "${CMAKE_SOURCE_DIR}/icons/hi*-app-kbibtex.png"
)

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtextest kbibtextest_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

kde4_add_executable(
    kbibtextest
    ${kbibtextest_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_link_libraries(
    kbibtextest
    LINK_PRIVATE
    ${QT_QTWEBKIT_LIBRARIES}
    ${KDE4_KIO_LIBS}
    ${KDE4_KPARTS_LIBS}
    ${KDE4_KFILE_LIBS}
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexproc
    kbibtexgui
    kbibtexnetworking
)


# creates version.h using cmake script
add_custom_target(
    GITrevision
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getgit.cmake
    COMMENT
    "Determine Git revision"
)

# version.h is a generated file
set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    PROPERTIES
    GENERATED
    TRUE
    HEADER_FILE_ONLY
    TRUE
)

add_dependencies(
    kbibtextest
    GITrevision
)
