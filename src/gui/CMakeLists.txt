# KBibTeX GUI library

include(
    CheckIncludeFiles
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/bibtex
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/element
    ${CMAKE_CURRENT_SOURCE_DIR}/field
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../networking/
    ${CMAKE_CURRENT_SOURCE_DIR}/../processing/
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
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
    widgets/filterbar.cpp
    widgets/radiobuttontreeview.cpp
    config/entrylayout.cpp
    preferences/kbibtexpreferencesdialog.cpp
    preferences/settingsgeneralwidget.cpp
    preferences/settingsglobalkeywordswidget.cpp
    preferences/settingscolorlabelwidget.cpp
    preferences/settingsuserinterfacewidget.cpp
    preferences/settingsfileexporterbibtexwidget.cpp
    preferences/settingsfileexporterpdfpswidget.cpp
    preferences/settingsfileexporterwidget.cpp
    preferences/settingsabstractwidget.cpp
    preferences/settingsidsuggestionswidget.cpp
    preferences/settingsidsuggestionseditor.cpp
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
    preferences/settingsfileexporterbibtexwidget.h
    preferences/settingsfileexporterpdfpswidget.h
    preferences/settingsidsuggestionswidget.h
    preferences/settingsidsuggestionseditor.h
    valuelistmodel.h
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
    -DMAKE_KBIBTEXGUI_LIB
)

# debug area for KBibTeX's GUI library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101012
)

kde4_add_library(
    kbibtexgui
    SHARED
    ${kbibtexgui_LIB_SRCS}
)

target_link_libraries(
    kbibtexgui
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    kbibtexio
    kbibtexnetworking
    kbibtexproc
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
