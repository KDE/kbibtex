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
)

# # debug area for KBibTeX's program
# add_definitions(
#     -DKDE_DEFAULT_DEBUG_AREA=101017
# )

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtex kbibtex_SRCS)
endif(UNITY_BUILD AND NOT WIN32)

include_directories(
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
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
    ${LIBXML2_INCLUDE_DIR}
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
    ${CMAKE_BINARY_DIR}/src/version.h
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

# creates version.h using cmake script
add_custom_target(
    GITrevisionProgram
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_BINARY_DIR}/src
    -P
    ${CMAKE_SOURCE_DIR}/src/getgit.cmake
    COMMENT
    "Determine Git revision"
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

add_dependencies(
    kbibtex
    GITrevisionProgram
)

install(
    TARGETS
    kbibtex
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
    PROGRAMS
    org.kde.kbibtex.desktop
    DESTINATION
    ${XDG_APPS_INSTALL_DIR}
)
install(
    FILES
    kbibtexui.rc
    DESTINATION
    ${KXMLGUI_INSTALL_DIR}/kbibtex
)

ecm_install_icons(
    ICONS
    ${CMAKE_SOURCE_DIR}/icons/128-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/16-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/22-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/32-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/48-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/64-apps-kbibtex.png
    DESTINATION ${ICON_INSTALL_DIR}
)
