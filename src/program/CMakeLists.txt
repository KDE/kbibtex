# KBibTeX main program

project(
    kbibtexprogram
)

set(
    kbibtex_SRCS
    program.cpp
    mainwindow.cpp
    documentlist.cpp
    mdiwidget.cpp
    docklets/statistics.cpp
    docklets/referencepreview.cpp
    docklets/documentpreview.cpp
    docklets/valuelist.cpp
    docklets/searchform.cpp
    docklets/searchresults.cpp
    docklets/elementform.cpp
    docklets/filesettings.cpp
    openfileinfo.cpp
    ${CMAKE_SOURCE_DIR}/src/global/kbibtex.cpp
    logging_program.cpp
)

if(BUILD_ZOTERO)
    list(
        APPEND
        kbibtex_SRCS
        docklets/zoterobrowser.cpp
        )
endif(BUILD_ZOTERO)


if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtex kbibtex_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_BINARY_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/processing
    ${CMAKE_BINARY_DIR}/src/processing
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_BINARY_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io/config
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_BINARY_DIR}/src/networking
    ${CMAKE_SOURCE_DIR}/src/networking/onlinesearch
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_BINARY_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/gui/file
    ${CMAKE_SOURCE_DIR}/src/gui/element
    ${CMAKE_SOURCE_DIR}/src/program/docklets
    ${CMAKE_SOURCE_DIR}/src/global
)

ecm_add_app_icon( kbibtex_SRCS
    ICONS
    ${CMAKE_SOURCE_DIR}/icons/128-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/16-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/22-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/32-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/48-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/64-apps-kbibtex.png
)

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
    kbibtex
    ${kbibtex_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtex-git-info.h
)

target_link_libraries( kbibtex
    Qt5::Core
    Qt5::Widgets
    KF5::CoreAddons
    KF5::I18n
    KF5::ConfigCore
    KF5::Service
    KF5::Parts
    KF5::IconThemes
    KF5::KIOCore
    KF5::KIOFileWidgets
    KF5::KIOWidgets
    KF5::KIONTLM
    KF5::Crash
    kbibtexio
    kbibtexgui
    kbibtexnetworking
)

if(Qt5WebEngineWidgets_FOUND)
    target_link_libraries( kbibtex
        Qt5::WebEngineWidgets
    )
else(Qt5WebEngineWidgets_FOUND)
    if(Qt5WebKitWidgets_FOUND)
        target_link_libraries( kbibtex
            Qt5::WebKitWidgets
        )
    endif(Qt5WebKitWidgets_FOUND)
endif(Qt5WebEngineWidgets_FOUND)

install(
    TARGETS
    kbibtex
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
    PROGRAMS
    org.kde.kbibtex.desktop
    DESTINATION
    ${KDE_INSTALL_APPDIR}
)
install(
    FILES
    kbibtexui.rc
    DESTINATION
    ${KDE_INSTALL_KXMLGUI5DIR}/kbibtex
)
install(
    FILES
    org.kde.kbibtex.appdata.xml
    DESTINATION
    ${KDE_INSTALL_METAINFODIR}
)

ecm_install_icons(
    ICONS
    ${CMAKE_SOURCE_DIR}/icons/128-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/16-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/22-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/32-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/48-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/64-apps-kbibtex.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
)
