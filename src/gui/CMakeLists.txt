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
    ${CMAKE_SOURCE_DIR}/src/gui/file
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
    file/fileview.cpp
    file/findduplicatesui.cpp
    file/clipboard.cpp
    file/filemodel.cpp
    file/basicfileview.cpp
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
    file/findduplicatesui.h
    file/basicfileview.h
    file/clipboard.h
    file/filemodel.h
    file/fileview.h
    element/findpdfui.h
    element/elementeditor.h
    element/associatedfilesui.h
)

add_definitions(
    -DMAKE_KBIBTEXGUI_LIB
)

# debug area for KBibTeX's GUI library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101012
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexgui kbibtexgui_LIB_SRCS)
endif(UNITY_BUILD)

if(QXT_FOUND_QXTCORE AND QXT_FOUND_QXTGUI)
    include_directories(
        ${QXT_QXTGUI_INCLUDE_DIR}
        ${QXT_QXTCORE_INCLUDE_DIR}
    )
    list(
        APPEND
        kbibtexgui_LIBS
        ${QXT_QXTGUI_LIB_RELEASE}
    )
else()
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/3rdparty/libqxt/gui
    )
    list(
        APPEND
        kbibtexgui_LIB_SRCS
        ${CMAKE_SOURCE_DIR}/src/3rdparty/libqxt/gui/qxtspanslider.cpp
    )
    add_definitions(
        -DBUILD_QXT_GUI
    )
endif()

kde4_add_library(
    kbibtexgui
    SHARED
    ${kbibtexgui_LIB_SRCS}
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
    ${kbibtexgui_LIBS}
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
    file/findduplicatesui.rc
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
