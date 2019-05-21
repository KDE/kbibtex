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
    docklets/zoterobrowser.cpp
    openfileinfo.cpp
    logging_program.cpp
)

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtex kbibtex_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/program/docklets
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

add_executable(
    kbibtex
    ${kbibtex_SRCS}
)

add_dependencies(kbibtex
    generate-kbibtex-git-info
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
    KF5::Wallet
    kbibtexio
    kbibtexgui
    kbibtexnetworking
    kbibtexprocessing
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
