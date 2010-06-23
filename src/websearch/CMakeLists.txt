# WebSearch library

set( websearch_LIB_SRCS
	websearchabstract.cpp
	websearchbibsonomy.cpp
)

add_definitions( -DMAKE_WEBSEARCH_LIB )
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
)

kde4_add_library( kbibtexws SHARED ${websearch_LIB_SRCS} )

target_link_libraries( kbibtexws 
	${QT_QTCORE_LIBRARY}
	${KDE4_KDECORE_LIBS}
	${KDE4_KIO_LIBS}
	kbibtexio
)

install(TARGETS kbibtexws DESTINATION ${LIB_INSTALL_DIR})

