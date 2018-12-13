# KBibTeX GUI library

set(
    kbibtexgui_LIB_SRCS
    field/fieldinput.cpp
    field/fieldlineedit.cpp
    field/fieldlistedit.cpp
    field/colorlabelwidget.cpp
    file/fileview.cpp
    file/filedelegate.cpp
    file/partwidget.cpp
    file/findduplicatesui.cpp
    file/clipboard.cpp
    file/basicfileview.cpp
    file/sortfilterfilemodel.cpp
    element/elementeditor.cpp
    element/elementwidgets.cpp
    element/findpdfui.cpp
    element/associatedfilesui.cpp
    widgets/menulineedit.cpp
    widgets/filesettingswidget.cpp
    widgets/filterbar.cpp
    widgets/radiobuttontreeview.cpp
    widgets/hidingtabwidget.cpp
    widgets/starrating.cpp
    widgets/rangewidget.cpp
    config/entrylayout.cpp
    preferences/kbibtexpreferencesdialog.cpp
    preferences/settingsgeneralwidget.cpp
    preferences/settingsglobalkeywordswidget.cpp
    preferences/settingscolorlabelwidget.cpp
    preferences/settingsuserinterfacewidget.cpp
    preferences/settingsfileexporterpdfpswidget.cpp
    preferences/settingsfileexporterwidget.cpp
    preferences/settingsabstractwidget.cpp
    preferences/settingsidsuggestionswidget.cpp
    preferences/settingsidsuggestionseditor.cpp
    guihelper.cpp
    italictextitemmodel.cpp
    valuelistmodel.cpp
    delayedexecutiontimer.cpp
    ${CMAKE_SOURCE_DIR}/src/global/kbibtex.cpp
    ${CMAKE_SOURCE_DIR}/src/global/preferences.cpp
    logging_gui.cpp
)

set(
    kbibtexgui_HDRS
    field/fieldinput.h
    field/colorlabelwidget.h
    field/fieldlineedit.h
    widgets/filterbar.h
    preferences/settingsuserinterfacewidget.h
    preferences/kbibtexpreferencesdialog.h
    preferences/settingsglobalkeywordswidget.h
    preferences/settingsfileexporterwidget.h
    preferences/settingsgeneralwidget.h
    preferences/settingsabstractwidget.h
    preferences/settingscolorlabelwidget.h
    preferences/settingsfileexporterpdfpswidget.h
    preferences/settingsidsuggestionswidget.h
    preferences/settingsidsuggestionseditor.h
    guihelper.h
    valuelistmodel.h
    italictextitemmodel.h
    delayedexecutiontimer.h
    file/findduplicatesui.h
    file/basicfileview.h
    file/clipboard.h
    file/fileview.h
    file/filedelegate.h
    file/sortfilterfilemodel.h
    file/partwidget.h
    element/findpdfui.h
    element/elementeditor.h
    element/associatedfilesui.h
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexgui kbibtexgui_LIB_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_BINARY_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_BINARY_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io/config
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_BINARY_DIR}/src/networking
    ${CMAKE_SOURCE_DIR}/src/processing
    ${CMAKE_BINARY_DIR}/src/processing
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_BINARY_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/file
    ${CMAKE_SOURCE_DIR}/src/gui/dialogs
    ${CMAKE_SOURCE_DIR}/src/gui/element
    ${CMAKE_SOURCE_DIR}/src/gui/field
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/gui/config
    ${CMAKE_SOURCE_DIR}/src/global
)

add_library(
    kbibtexgui
    SHARED
    ${kbibtexgui_LIB_SRCS}
)

target_link_libraries( kbibtexgui
    Qt5::Core
    KF5::IconThemes
    KF5::ItemViews
    KF5::Completion
    KF5::TextEditor
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexnetworking
    kbibtexproc
)

set_target_properties(
    kbibtexgui
    PROPERTIES
    EXPORT_NAME "kbibtexgui"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

install(
    TARGETS
    kbibtexgui
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexgui )
