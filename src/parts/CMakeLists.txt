set(
    kbibtexpart_SRCS
    part.cpp
    partfactory.cpp
    logging_parts.cpp
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexpart kbibtexpart_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexpart
    MODULE
    ${kbibtexpart_SRCS}
)

add_dependencies(kbibtexpart
    generate-kbibtex-git-info
)

target_include_directories(kbibtexpart
    PRIVATE
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(kbibtexpart
    PRIVATE
        KF5::Parts
        KF5::CoreAddons
        KF5::I18n
        KBibTeX::GUI
        KBibTeX::IO
        KBibTeX::Networking
        KBibTeX::Processing
)

install(
    TARGETS
    kbibtexpart
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
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
