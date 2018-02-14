# KBibTeX KPart

set(
    kbibtexpart_SRCS
    part.cpp
    partfactory.cpp
    browserextension.cpp
    ${CMAKE_SOURCE_DIR}/src/global/preferences.cpp
    logging_parts.cpp
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexpart kbibtexpart_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_BINARY_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/config
    ${CMAKE_SOURCE_DIR}/src/gui/element
    ${CMAKE_SOURCE_DIR}/src/gui/preferences
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/gui/file
    ${CMAKE_SOURCE_DIR}/src/processing
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_SOURCE_DIR}/src/global
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

add_library( kbibtexpart
    MODULE
    ${kbibtexpart_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtex-git-info.h
)

target_link_libraries( kbibtexpart
    KF5::Parts
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexgui
    kbibtexproc
)

install(
    TARGETS
    kbibtexpart
    DESTINATION
    ${KDE_INSTALL_PLUGINDIR}
)
kcoreaddons_desktop_to_json(kbibtexpart kbibtexpart.desktop SERVICE_TYPES kpart.desktop)
install(
    FILES
    kbibtexpart.desktop
    DESTINATION
    ${KDE_INSTALL_KSERVICES5DIR}
)
install(
    FILES
    kbibtexpartui.rc
    DESTINATION
    ${KDE_INSTALL_KXMLGUI5DIR}/kbibtexpart
)
