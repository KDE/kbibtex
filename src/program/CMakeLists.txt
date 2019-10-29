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
    docklets/zoterobrowser.cpp
    openfileinfo.cpp
)

ecm_qt_declare_logging_category(kbibtexprogram_SRCS
    HEADER logging_program.h
    IDENTIFIER LOG_KBIBTEX_PROGRAM
    CATEGORY_NAME kbibtex.program
)

if(UNITY_BUILD AND NOT WIN32) # FIXME: Unity build of programs breaks on Windows
    enable_unity_build(kbibtex kbibtexprogram_SRCS)
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
        Qt5::Core
        KF5::CoreAddons
        KF5::Crash
        KF5::I18n
        KF5::IconThemes
        KF5::KIOCore
        KF5::KIOFileWidgets
        KF5::Parts
        KF5::Wallet
        KBibTeX::Global
        KBibTeX::GUI
        KBibTeX::IO
        KBibTeX::Networking
        KBibTeX::Processing
)

if(Qt5WebEngineWidgets_FOUND)
    message(STATUS "Using QtWebEngine to render complex HTML content")
    # Once CMake 3.12.x is minimum requirement, use 'add_compile_definitions'
    add_definitions(
        -DHAVE_WEBENGINEWIDGETS
    )
    target_link_libraries(kbibtex
        PRIVATE
            Qt5::WebEngineWidgets
    )
else()
    if(Qt5WebKitWidgets_FOUND)
        message(STATUS "Using QtWebKit to render complex HTML content")
        # Once CMake 3.12.x is minimum requirement, use 'add_compile_definitions'
        add_definitions(
            -DHAVE_WEBKITWIDGETS
        )
        target_link_libraries(kbibtex
            PRIVATE
                Qt5::WebKitWidgets
         )
    else()
        message(STATUS "If available, using a KPart to render complex HTML content")
    endif()
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
    kbibtexui.rc
    DESTINATION
    ${KDE_INSTALL_KXMLGUI5DIR}/kbibtex
)
install(
    FILES
    org.kde.kbibtex.appdata.xml
    DESTINATION
    ${KDE_INSTALL_METAINFODIR}
)

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
