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
    docklets/zotero.cpp
    openfileinfo.cpp
)

# debug area for KBibTeX's IO library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101014
)

kde4_add_app_icon(
    kbibtex_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../icons/hi*-app-kbibtex.png"
)

# a custom target that is always built
add_custom_target(
    set_version_h_in_builddir
    ALL
    DEPENDS
    svn_header
)

# creates src/version.h using cmake script
add_custom_command(
    OUTPUT
    svn_header
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getsvn.cmake
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

# make sure that program.cpp gets recompiled if SVN version changes
add_file_dependencies(
    program.cpp
    ${CMAKE_BINARY_DIR}/src/version.h
)

kde4_add_executable(
    kbibtex${BINARY_POSTFIX}
    ${kbibtex_SRCS}
)

# add_dependencies(kbibtex${BINARY_POSTFIX} set_version_h_in_builddir)
# add_dependencies(kbibtex${BINARY_POSTFIX} ${CMAKE_BINARY_DIR}/src/version.h)

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
