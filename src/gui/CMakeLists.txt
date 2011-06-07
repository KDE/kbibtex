# KBibTeX GUI library

include( CheckIncludeFiles )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/bibtex
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/element
    ${CMAKE_CURRENT_SOURCE_DIR}/field
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../processing/
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
)

set( kbibtexgui_LIB_SRCS
    field/fieldinput.cpp
    field/fieldlineedit.cpp
    field/fieldlistedit.cpp
    bibtex/bibtexeditor.cpp
    bibtex/findduplicatesui.cpp
    bibtex/clipboard.cpp
    bibtex/bibtexfilemodel.cpp
    bibtex/bibtexfileview.cpp
    element/elementeditor.cpp
    element/elementwidgets.cpp
    widgets/menulineedit.cpp
    widgets/filterbar.cpp
    widgets/radiobuttontreeview.cpp
    config/entrylayout.cpp
    valuelistmodel.cpp
)

add_definitions( -DMAKE_KBIBTEXGUI_LIB )

# debug area for KBibTeX's GUI library
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101012)

kde4_add_library( kbibtexgui SHARED ${kbibtexgui_LIB_SRCS} )

target_link_libraries( kbibtexgui
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KIO_LIBS}
    kbibtexio
    kbibtexproc
)

install(TARGETS kbibtexgui RUNTIME DESTINATION bin LIBRARY DESTINATION ${LIB_INSTALL_DIR})

install( FILES bibtex/findduplicatesui.rc  DESTINATION ${DATA_INSTALL_DIR}/kbibtex )
