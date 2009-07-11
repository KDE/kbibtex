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
	config/bibtexfields.cpp
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


# KBibTeX widget test program

set( widgettestprogram_SRCS
	main.cpp
	mainwindow.cpp
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../lib
)

kde4_add_executable( widgettestprogram ${widgettestprogram_SRCS} )

target_link_libraries( widgettestprogram
	${KDE4_KPARTS_LIBS}
	kbibtexio
	kbibtexgui
)

install( TARGETS widgettestprogram ${INSTALL_TARGETS_DEFAULT_ARGS} )
