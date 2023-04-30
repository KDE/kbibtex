# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2009-2023 Thomas Fischer <fischer@unix-ag.uni-kl.de>

configure_file(kbibtexpart.json.in ${CMAKE_CURRENT_BINARY_DIR}/kbibtexpart.json @ONLY)

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
