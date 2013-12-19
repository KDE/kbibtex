# KBibTeX GUI library

include(
    CheckIncludeFiles
)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io/config
    ${CMAKE_SOURCE_DIR}/src/networking
    ${CMAKE_SOURCE_DIR}/src/processing
    ${CMAKE_SOURCE_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui/bibtex
    ${CMAKE_SOURCE_DIR}/src/gui/dialogs
    ${CMAKE_SOURCE_DIR}/src/gui/element
    ${CMAKE_SOURCE_DIR}/src/gui/field
    ${CMAKE_SOURCE_DIR}/src/gui/widgets
    ${CMAKE_SOURCE_DIR}/src/gui/config
)

set(
    kbibtexgui_LIB_SRCS
    field/fieldinput.cpp
    field/fieldlineedit.cpp
    field/fieldlistedit.cpp
    field/colorlabelwidget.cpp
    bibtex/bibtexeditor.cpp
    bibtex/findduplicatesui.cpp
    bibtex/clipboard.cpp
    bibtex/bibtexfilemodel.cpp
    bibtex/bibtexfileview.cpp
    element/elementeditor.cpp
    element/elementwidgets.cpp
    element/findpdfui.cpp
    widgets/menulineedit.cpp
    widgets/filesettingswidget.cpp
    widgets/filterbar.cpp
    widgets/radiobuttontreeview.cpp
    widgets/hidingtabwidget.cpp
    widgets/starrating.cpp
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
    kbibtexgui_export.h
    bibtex/findduplicatesui.h
    bibtex/bibtexfileview.h
    bibtex/clipboard.h
    bibtex/bibtexfilemodel.h
    bibtex/bibtexeditor.h
    element/findpdfui.h
    element/elementeditor.h
)

add_definitions(
    -DMAKE_KBIBTEXGUI_LIB -DBUILD_QXT_GUI
)

# debug area for KBibTeX's GUI library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101012
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexgui kbibtexgui_LIB_SRCS)
endif(UNITY_BUILD)

kde4_add_library(
    kbibtexgui
    SHARED
    ${kbibtexgui_LIB_SRCS}
    ${CMAKE_SOURCE_DIR}/src/3rdparty/libqxt/gui/qxtspanslider.cpp
)

target_link_libraries(
    kbibtexgui
    LINK_PRIVATE
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    kbibtexconfig
    kbibtexdata
    kbibtexio
    kbibtexnetworking
    kbibtexproc
)

set_target_properties(
    kbibtexgui
    PROPERTIES
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexgui
    RUNTIME
    DESTINATION
    bin
    LIBRARY
    DESTINATION
    ${LIB_INSTALL_DIR}
)

install(
    FILES
    bibtex/findduplicatesui.rc
    DESTINATION
    ${DATA_INSTALL_DIR}/kbibtex
)

install(
    FILES
    ${kbibtexgui_HDRS}
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex
    COMPONENT
    Devel
)
