project(
    kbibtextest
)

include(
    AddFileDependencies
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/docklets
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/bibtex
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/element
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking
    ${CMAKE_CURRENT_SOURCE_DIR}/../websearch
    ${CMAKE_CURRENT_SOURCE_DIR}/../processing
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
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
    kbibtexio
    kbibtexproc
    kbibtexgui
    kbibtexws
)
