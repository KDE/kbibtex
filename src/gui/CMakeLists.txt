# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2009-2021 Thomas Fischer <fischer@unix-ag.uni-kl.de>
# SPDX-FileContributor: 2013 Pino Toscano <pino@kde.org>
# SPDX-FileContributor: 2019 Jonathan Riddell <jr@jriddell.org>

set(
    kbibtexgui_SRCS
    element/associatedfilesui.cpp
    element/elementeditor.cpp
    element/elementwidgets.cpp
    element/findpdfui.cpp
    field/colorlabelwidget.cpp
    field/fieldinput.cpp
    field/fieldlineedit.cpp
    field/fieldlistedit.cpp
    file/basicfileview.cpp
    file/clipboard.cpp
    file/filedelegate.cpp
    file/fileview.cpp
    file/partwidget.cpp
    file/findduplicatesui.cpp
    file/sortfilterfilemodel.cpp
    widgets/filesettingswidget.cpp
    widgets/filterbar.cpp
    widgets/hidingtabwidget.cpp
    widgets/menulineedit.cpp
    widgets/radiobuttontreeview.cpp
    widgets/rangewidget.cpp
    widgets/starrating.cpp
    config/entrylayout.cpp
    preferences/kbibtexpreferencesdialog.cpp
    preferences/settingsabstractwidget.cpp
    preferences/settingscolorlabelwidget.cpp
    preferences/settingsfileexporterpdfpswidget.cpp
    preferences/settingsfileexporterwidget.cpp
    preferences/settingsgeneralwidget.cpp
    preferences/settingsglobalkeywordswidget.cpp
    preferences/settingsidsuggestionswidget.cpp
    preferences/settingsidsuggestionseditor.cpp
    preferences/settingsuserinterfacewidget.cpp
    guihelper.cpp
    italictextitemmodel.cpp
    valuelistmodel.cpp
)

ecm_qt_declare_logging_category(kbibtexgui_SRCS
    HEADER logging_gui.h
    IDENTIFIER LOG_KBIBTEX_GUI
    CATEGORY_NAME kbibtex.gui
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexgui kbibtexgui_SRCS)
endif(UNITY_BUILD)

add_library(kbibtexgui
    SHARED
    ${kbibtexgui_SRCS}
)
generate_export_header(kbibtexgui)
add_library(KBibTeX::GUI ALIAS kbibtexgui)

set_target_properties(kbibtexgui
    PROPERTIES
    EXPORT_NAME "kbibtexgui"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

target_include_directories(kbibtexgui
    INTERFACE
        $<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KBibTeX/gui>
)

target_link_libraries(kbibtexgui
    PUBLIC
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Widgets
    PRIVATE
        Poppler::Qt${QT_MAJOR_VERSION}
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Concurrent
        KF${QT_MAJOR_VERSION}::ConfigCore
        KF${QT_MAJOR_VERSION}::I18n
        KF${QT_MAJOR_VERSION}::IconThemes
        KF${QT_MAJOR_VERSION}::WidgetsAddons
        KF${QT_MAJOR_VERSION}::KIOCore
        KF${QT_MAJOR_VERSION}::KIOFileWidgets
        KF${QT_MAJOR_VERSION}::Parts
        KF${QT_MAJOR_VERSION}::TextEditor
        KF${QT_MAJOR_VERSION}::TextWidgets
        KBibTeX::Config
        KBibTeX::Data
        KBibTeX::Global
        KBibTeX::IO
        KBibTeX::Networking
        KBibTeX::Processing
)

if(Qt5XmlPatterns_FOUND)
    target_link_libraries(kbibtexio
        PRIVATE
            Qt5::XmlPatterns
    )
endif()

install(
    TARGETS kbibtexgui
    EXPORT kbibtexgui-targets
    LIBRARY NAMELINK_SKIP
    ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

set_target_properties(kbibtexgui PROPERTIES
    EXPORT_NAME "GUI"
)

ecm_generate_headers(kbibtexgui_HEADERS
    HEADER_NAMES
        element/AssociatedFilesUI
        element/ElementEditor
        element/ElementWidgets
        element/FindPDFUI
        field/ColorLabelWidget
        field/FieldInput
        field/FieldLineEdit
        field/FieldListEdit
        file/BasicFileView
        file/Clipboard
        file/FileDelegate
        file/FileView
        file/FindDuplicatesUI
        file/PartWidget
        file/SortFilterFileModel
        widgets/FileSettingsWidget
        widgets/FilterBar
        widgets/HidingTabWidget
        widgets/MenuLineEdit
        widgets/RadioButtonTreeView
        widgets/RangeWidget
        widgets/StarRating
        config/EntryLayout
        preferences/KBibTeXPreferencesDialog
        preferences/SettingsAbstractWidget
        preferences/SettingsColorLabelWidget
        preferences/SettingsFileExporterPDFPSWidget
        preferences/SettingsFileExporterWidget
        preferences/SettingsGeneralWidget
        preferences/SettingsGlobalKeywordsWidget
        preferences/SettingsIdSuggestionsEditor
        preferences/SettingsIdSuggestionsWidget
        preferences/SettingsUserInterfaceWidget
        ItalicTextItemModel
        ValueListModel
        GUIHelper
    REQUIRED_HEADERS kbibtexgui_HEADERS
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kbibtexgui_export.h
    ${kbibtexgui_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KBibTeX/gui
    COMPONENT Devel
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGUI-configVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/KBibTeXGUI-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGUI-config.cmake
    INSTALL_DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGUI-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KBibTeXGUI-configVersion.cmake
    DESTINATION ${KDE_INSTALL_LIBDIR}/cmake/KBibTeX
)
