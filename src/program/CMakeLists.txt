# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2009-2022 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2017 Pino Toscano <pino@kde.org>

set(
    kbibtexprogram_SRCS
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

    program.qrc
)

if(Qt${QT_MAJOR_VERSION}NetworkAuth_FOUND)
    set(
        kbibtexprogram_SRCS
        ${kbibtexprogram_SRCS}
        docklets/zoterobrowser.cpp
    )
endif()

set(
    kbibtexcliprogram_SRCS
    commandline.cpp
)

ecm_qt_declare_logging_category(kbibtexprogram_SRCS
    HEADER logging_program.h
    IDENTIFIER LOG_KBIBTEX_PROGRAM
    CATEGORY_NAME kbibtex.program
)

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtex kbibtexprogram_SRCS)
    enable_unity_build(kbibtex kbibtexcliprogram_SRCS)
endif(UNITY_BUILD AND NOT WIN32)


ecm_add_app_icon(kbibtexprogram_SRCS
    ICONS
    ${CMAKE_SOURCE_DIR}/icons/16-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/22-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/32-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/48-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/64-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/128-apps-kbibtex.png
)

if(BUILD_APP_DESKTOP)
    add_executable(kbibtex
        ${kbibtexprogram_SRCS}
    )

    add_dependencies(kbibtex
        generate-kbibtex-git-info
    )

    target_include_directories(kbibtex
        PRIVATE
            ${CMAKE_BINARY_DIR}
    )

    target_link_libraries(kbibtex
        PRIVATE
            Qt${QT_MAJOR_VERSION}::Core
            KF${QT_MAJOR_VERSION}::CoreAddons
            KF${QT_MAJOR_VERSION}::Crash
            KF${QT_MAJOR_VERSION}::I18n
            KF${QT_MAJOR_VERSION}::IconThemes
            KF${QT_MAJOR_VERSION}::KIOCore
            KF${QT_MAJOR_VERSION}::KIOFileWidgets
            KF${QT_MAJOR_VERSION}::Parts
            KF${QT_MAJOR_VERSION}::Wallet
            KF${QT_MAJOR_VERSION}::TextWidgets
            KBibTeX::Global
            KBibTeX::GUI
            KBibTeX::IO
            KBibTeX::Networking
            KBibTeX::Processing
    )

    if(Qt${QT_MAJOR_VERSION}WebEngineWidgets_FOUND)
        message(STATUS "Using QtWebEngine to render complex HTML content")
        target_link_libraries(kbibtex
            PRIVATE
                Qt${QT_MAJOR_VERSION}::WebEngineWidgets
        )
    else()
        message(STATUS "If available, using a KPart to render complex HTML content")
    endif()

    install(
        TARGETS
        kbibtex
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
    )

    install(
        PROGRAMS
        org.kde.kbibtex.desktop
        DESTINATION
        ${KDE_INSTALL_APPDIR}
    )
    install(
        FILES
        org.kde.kbibtex.appdata.xml
        DESTINATION
        ${KDE_INSTALL_METAINFODIR}
    )
endif(BUILD_APP_DESKTOP)

if(BUILD_APP_COMMAND_LINE)
    add_executable(kbibtex-cli
        ${kbibtexcliprogram_SRCS}
    )

    target_include_directories(kbibtex-cli
        PRIVATE
            ${CMAKE_BINARY_DIR}
    )

    target_link_libraries(kbibtex-cli
        PRIVATE
            Qt${QT_MAJOR_VERSION}::Core
            KBibTeX::Data
            KBibTeX::IO
            KBibTeX::Processing
    )

    install(
        TARGETS
        kbibtex-cli
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
    )
endif(BUILD_APP_COMMAND_LINE)

ecm_install_icons(
    ICONS
    ${CMAKE_SOURCE_DIR}/icons/16-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/22-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/32-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/48-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/64-apps-kbibtex.png
    ${CMAKE_SOURCE_DIR}/icons/128-apps-kbibtex.png
    DESTINATION ${KDE_INSTALL_ICONDIR}
)
