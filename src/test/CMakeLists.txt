project(
    kbibtextest
)

include(
    AddFileDependencies
)

add_definitions(
    -DTESTSET_DIRECTORY='\"${CMAKE_SOURCE_DIR}/testset/\"'
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
)

kde4_add_executable(
    kbibtextest
    ${kbibtextest_SRCS}
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
