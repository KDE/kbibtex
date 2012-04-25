include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/../gui/bibtex
    ${CMAKE_CURRENT_SOURCE_DIR}/../processing/
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

# creates src/version.h using cmake script
add_custom_target(
    SVNrevisionForPart
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -P
    ${CMAKE_SOURCE_DIR}/src/getsvn.cmake
    COMMENT
    "Determine SVN version"
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

kde4_add_plugin(
    kbibtexpart
    ${kbibtexpart_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_link_libraries(
    kbibtexpart
    ${KDE4_KPARTS_LIBS}
    kbibtexio
    kbibtexgui
    kbibtexproc
)

add_dependencies(
    kbibtexpart
    SVNrevisionForPart
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

