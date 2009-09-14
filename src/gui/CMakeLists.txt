# KBibTeX GUI library

include( CheckIncludeFiles )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/widgets
	${CMAKE_CURRENT_SOURCE_DIR}/config
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
)

set( kbibtexgui_LIB_SRCS
	widgets/bibtexfileview.cpp
	widgets/bibtexfilemodel.cpp
	widgets/fieldeditor.cpp
	config/bibtexfields.cpp
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

