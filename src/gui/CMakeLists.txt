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
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
    ${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
)

set( kbibtexgui_LIB_SRCS
    field/fieldinput.cpp
    field/fieldlineedit.cpp
    field/fieldlistedit.cpp
    bibtex/bibtexeditor.cpp
    bibtex/clipboard.cpp
    bibtex/bibtexfilemodel.cpp
    bibtex/bibtexfileview.cpp
    element/elementeditor.cpp
    element/elementwidgets.cpp
    widgets/menulineedit.cpp
    widgets/filterbar.cpp
    config/entrylayout.cpp
    valuelistmodel.cpp
)

add_definitions( -DMAKE_KBIBTEXGUI_LIB )

kde4_add_library( kbibtexgui SHARED ${kbibtexgui_LIB_SRCS} )

target_link_libraries( kbibtexgui
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${KDE4_KDEUI_LIBS}
        ${KDE4_KIO_LIBS}
        kbibtexio
)

install(TARGETS kbibtexgui DESTINATION ${LIB_INSTALL_DIR})

