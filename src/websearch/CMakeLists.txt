# WebSearch library

set( websearch_LIB_SRCS
	websearchabstract.cpp
	websearchbibsonomy.cpp
	websearcharxiv.cpp
	websearchgooglescholar.cpp
	websearchieeexplore.cpp
	websearchpubmed.cpp
	websearchgeneral.cpp
)

add_definitions( -DMAKE_WEBSEARCH_LIB )

# debug area for KBibTeX's web search library
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101015)

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

