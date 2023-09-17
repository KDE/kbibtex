# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2009-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>

configure_file(kbibtexpart.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/kbibtexpart.desktop @ONLY)

set(
    kbibtexpart_SRCS
    part.cpp

    part.qrc
)

ecm_qt_declare_logging_category(kbibtexpart_SRCS
    HEADER logging_part.h
    IDENTIFIER LOG_KBIBTEX_PART
    CATEGORY_NAME kbibtex.part
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
    DESTINATION
    ${KDE_INSTALL_PLUGINDIR}
)

kcoreaddons_desktop_to_json(kbibtexpart ${CMAKE_CURRENT_BINARY_DIR}/kbibtexpart.desktop)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexpart.desktop
    DESTINATION
    ${KDE_INSTALL_KSERVICES5DIR}
)
