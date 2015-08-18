include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/config
    ${CMAKE_SOURCE_DIR}/src/gui/element
    ${CMAKE_SOURCE_DIR}/src/gui/preferences
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/gui/file
    ${CMAKE_SOURCE_DIR}/src/processing
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_CURRENT_BINARY_DIR}
)

# debug area for KBibTeX's IO library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101013
)

set(
    kbibtexpart_SRCS
    part.cpp
    browserextension.cpp
    partfactory.cpp
)


# creates version.h using cmake script
add_custom_target(
    kbibtexpart-GITrevision
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getgit.cmake
    COMMENT
    "Determine Git revision"
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


if(UNITY_BUILD)
    enable_unity_build(kbibtexpart kbibtexpart_SRCS)
endif(UNITY_BUILD)

kde4_add_plugin(
    kbibtexpart
    ${kbibtexpart_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_link_libraries(
    kbibtexpart
    LINK_PRIVATE
    ${KDE4_KPARTS_LIBS}
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexgui
    kbibtexproc
)

add_dependencies(
    kbibtexpart
    kbibtexpart-GITrevision
)

install(
    TARGETS
    kbibtexpart
    DESTINATION
    ${PLUGIN_INSTALL_DIR}
)
install(
    FILES
    kbibtexpart.desktop
    DESTINATION
    ${SERVICES_INSTALL_DIR}
)
install(
    FILES
    kbibtexpartui.rc
    DESTINATION
    ${DATA_INSTALL_DIR}/kbibtexpart
)

