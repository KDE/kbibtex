project(
    kbibtextest
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
    ${CMAKE_BINARY_DIR}/src
)

set(
    kbibtextest_SRCS
    main.cpp
    kbibtextest.cpp
)


# creates src/version.h using cmake script
add_custom_target(
    SVNrevisionForPart
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getsvn.cmake
    COMMENT
    "Determine SVN version"
)

# version.h is a generated file
set_source_files_properties(
    ${CMAKE_BINARY_DIR}/src/version.h
    PROPERTIES
    GENERATED
    TRUE
    HEADER_FILE_ONLY
    TRUE
)


kde4_add_executable(
    kbibtextest
    ${kbibtextest_SRCS}
    ${CMAKE_BINARY_DIR}/src/version.h
)

add_dependencies(
    kbibtextest
    SVNrevisionForProgram
)
target_link_libraries(
    kbibtextest
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
