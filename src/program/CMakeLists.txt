project(
    kbibtexprogram
)

include(
    AddFileDependencies
)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io/config
    ${CMAKE_SOURCE_DIR}/src/processing
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/config
    ${CMAKE_SOURCE_DIR}/src/gui/bibtex
    ${CMAKE_SOURCE_DIR}/src/gui/element
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_SOURCE_DIR}/src/networking/onlinesearch
    ${CMAKE_CURRENT_SOURCE_DIR}/docklets
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBXML2_INCLUDE_DIR}
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
)

# debug area for KBibTeX's program
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101017
)

kde4_add_app_icon(
    kbibtex_SRCS
    "${CMAKE_SOURCE_DIR}/icons/hi*-app-kbibtex.png"
)

kde4_add_executable(
    kbibtex${BINARY_POSTFIX}
    ${kbibtex_SRCS}
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

target_link_libraries(
    kbibtex${BINARY_POSTFIX}
    ${QT_QTWEBKIT_LIBRARIES}
    ${KDE4_KIO_LIBS}
    ${KDE4_KPARTS_LIBS}
    ${KDE4_KFILE_LIBS}
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexproc
    kbibtexgui
    kbibtexnetworking
)


# creates version.h using cmake script
add_custom_target(
    SVNrevisionForProgram
    COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
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

add_dependencies(
    kbibtex${BINARY_POSTFIX}
    SVNrevisionForProgram
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
