# KBibTeX GUI library

include( CheckIncludeFiles )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/dialogs
	${CMAKE_CURRENT_SOURCE_DIR}/widgets
	${CMAKE_CURRENT_SOURCE_DIR}/config
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
)

set( kbibtexgui_LIB_SRCS
	widgets/bibtexfileview.cpp
	widgets/bibtexfilemodel.cpp
	widgets/menulineedit.cpp
	widgets/fieldlineedit.cpp
	widgets/fieldeditor.cpp
	widgets/entrylistdelegate.cpp
	widgets/entrylistview.cpp
	widgets/entrylistmodel.cpp
	widgets/filterbar.cpp
	config/bibtexfields.cpp
	dialogs/entryviewer.cpp
	dialogs/entryeditor.cpp
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

