# KBibTeX KPart

set(
    kbibtexpart_SRCS
    part.cpp
    partfactory.cpp
    browserextension.cpp
    logging_parts.cpp
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexpart kbibtexpart_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_library( kbibtexpart
    MODULE
    ${kbibtexpart_SRCS}
)

add_dependencies(kbibtexpart
    generate-kbibtex-git-info
)

target_link_libraries( kbibtexpart
    KF5::Parts
    KF5::CoreAddons
    KF5::ItemViews
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexgui
    kbibtexprocessing
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
