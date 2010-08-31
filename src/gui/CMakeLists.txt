# KBibTeX GUI library

include( CheckIncludeFiles )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/dialogs
	${CMAKE_CURRENT_SOURCE_DIR}/widgets
	${CMAKE_CURRENT_SOURCE_DIR}/widgets/fieldinput
	${CMAKE_CURRENT_SOURCE_DIR}/config
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/config
)

set( kbibtexgui_LIB_SRCS
	widgets/bibtexfileview.cpp
	widgets/bibtexfilemodel.cpp
	widgets/menulineedit.cpp
	widgets/fieldinput/fieldinput.cpp
	widgets/fieldinput/fieldlineedit.cpp
	widgets/fieldinput/fieldlistedit.cpp
	widgets/filterbar.cpp
	config/entrylayout.cpp
	dialogs/macroeditor.cpp
	dialogs/entryeditor.cpp
	clipboard.cpp
	bibtexeditor.cpp
)

add_definitions( -DMAKE_KBIBTEXGUI_LIB )

kde4_add_library( kbibtexgui SHARED ${kbibtexgui_LIB_SRCS} )

target_link_libraries( kbibtexgui
	${QT_QTCORE_LIBRARY}
	${QT_QTGUI_LIBRARY}
	${KDE4_KDEUI_LIBS}
	kbibtexio
)

install(TARGETS kbibtexgui DESTINATION ${LIB_INSTALL_DIR})

