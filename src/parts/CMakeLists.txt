include_directories(
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/config
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/gui/bibtex
    ${CMAKE_SOURCE_DIR}/src/processing
    ${CMAKE_SOURCE_DIR}/src/networking
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

kde4_add_plugin(
    kbibtexpart
    ${kbibtexpart_SRCS}
)

target_link_libraries(
    kbibtexpart
    ${KDE4_KPARTS_LIBS}
    kbibtexdata
    kbibtexio
    kbibtexgui
    kbibtexproc
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

