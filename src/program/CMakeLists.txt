project(
    kbibtexprogram
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
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking/onlinesearch
    ${CMAKE_CURRENT_SOURCE_DIR}/../processing
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../../
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

set(
    kbibtex_SRCS
    program.cpp
    mainwindow.cpp
    documentlist.cpp
    mdiwidget.cpp
    docklets/referencepreview.cpp
    docklets/documentpreview.cpp
    docklets/valuelist.cpp
    docklets/searchform.cpp
    docklets/searchresults.cpp
    docklets/elementform.cpp
    openfileinfo.cpp
)

# debug area for KBibTeX's IO library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101014
)

# creates src/version.h using cmake script
add_custom_command(
    OUTPUT
    ${CMAKE_BINARY_DIR}/src/version.h
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getsvn.cmake
    COMMENT
    "Determine SVN version"
)

# src/version.h is a generated file
set_source_files_properties(
    ${CMAKE_BINARY_DIR}/src/version.h
    PROPERTIES
    GENERATED
    TRUE
    HEADER_FILE_ONLY
    TRUE
)

kde4_add_app_icon(
    kbibtex_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../icons/hi*-app-kbibtex.png"
)

kde4_add_executable(
    kbibtex${BINARY_POSTFIX}
    ${kbibtex_SRCS}
    ${CMAKE_BINARY_DIR}/src/version.h
)

target_link_libraries(
    kbibtex${BINARY_POSTFIX}
    ${QT_QTWEBKIT_LIBRARIES}
    ${KDE4_KIO_LIBS}
    ${KDE4_KPARTS_LIBS}
    ${KDE4_KFILE_LIBS}
    kbibtexio
    kbibtexproc
    kbibtexgui
    kbibtexnetworking
)

install(
    TARGETS
    kbibtex${BINARY_POSTFIX}
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
    FILES
    kbibtex.desktop
    DESTINATION
    ${XDG_APPS_INSTALL_DIR}
)
install(
    FILES
    kbibtexui.rc
    DESTINATION
    ${DATA_INSTALL_DIR}/kbibtex
)
