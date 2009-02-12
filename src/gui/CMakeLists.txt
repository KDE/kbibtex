include( CheckIncludeFiles )

include_directories(
	${QT_INCLUDES}
	${KDE4_INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}/widgets
	${CMAKE_CURRENT_SOURCE_DIR}/config
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio/
)

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../lib
)


set( guitest_SRCS
	main.cpp
	widgets/bibtexfileview.cpp
	widgets/bibtexfilemodel.cpp
	config/bibtexfields.cpp
)

kde4_add_executable( guitest ${guitest_SRCS} )

target_link_libraries( guitest
	${KDE4_KDEUI_LIBS}
	kbibtexio
)

install( TARGETS guitest ${INSTALL_TARGETS_DEFAULT_ARGS} )
