# KBibTeX KPart

set(
    kbibtexpart_SRCS
    part.cpp
    browserextension.cpp
)

# # debug area for KBibTeX's IO library
# add_definitions(
#     -DKDE_DEFAULT_DEBUG_AREA=101013
# )

# if(UNITY_BUILD)
#     enable_unity_build(kbibtexpart kbibtexpart_SRCS)
# endif(UNITY_BUILD)

include_directories(
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
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# version.h is a generated file
set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    PROPERTIES
    GENERATED
    TRUE
    HEADER_FILE_ONLY
    TRUE
)

add_library( kbibtexpart
    MODULE
    ${kbibtexpart_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_link_libraries( kbibtexpart
    KF5::Parts
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexgui
    kbibtexproc
)

add_dependencies( kbibtexpart
    GITrevisionPart
)

# creates version.h using cmake script
add_custom_target(
    GITrevisionPart
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getgit.cmake
    COMMENT
    "Determine Git revision"
)

install(
    TARGETS
    kbibtexpart
    DESTINATION
    ${KDE_INSTALL_PLUGINDIR}
)
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
    ${KXMLGUI_INSTALL_DIR}/kbibtex
)
