project(
    kbibtexprogram
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

kde4_add_executable(
    kbibtex${BINARY_POSTFIX}
    ${kbibtex_SRCS}
)

add_dependencies(kbibtex${BINARY_POSTFIX} set_version_h_in_builddir)


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
