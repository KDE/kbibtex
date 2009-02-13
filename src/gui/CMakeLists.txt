include( CheckIncludeFiles )

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/widgets
	${CMAKE_CURRENT_SOURCE_DIR}/config
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../lib
)

# KBibTeX main program

set( kbibtex_SRCS
	main.cpp
	mainwindow.cpp
	widgets/bibtexfileview.cpp
	widgets/bibtexfilemodel.cpp
	config/bibtexfields.cpp
)

kde4_add_executable( kbibtex ${kbibtex_SRCS} )

target_link_libraries( kbibtex
	${KDE4_KDEUI_LIBS}
	${KDE4_KPARTS_LIBS}
	kbibtexio
)

install( TARGETS kbibtex ${INSTALL_TARGETS_DEFAULT_ARGS} )
