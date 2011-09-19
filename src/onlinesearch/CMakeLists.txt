# OnlineSearch library

set( onlinesearch_LIB_SRCS
	onlinesearchabstract.cpp
	onlinesearchbibsonomy.cpp
	onlinesearcharxiv.cpp
	onlinesearchsciencedirect.cpp
	onlinesearchgooglescholar.cpp
	onlinesearchieeexplore.cpp
	onlinesearchpubmed.cpp
	onlinesearchacmportal.cpp
	onlinesearchspringerlink.cpp
	onlinesearchjstor.cpp
	onlinesearchgeneral.cpp
)

add_definitions( -DMAKE_ONLINESEARCH_LIB )

# debug area for KBibTeX's web search library
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101015)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
)

kde4_add_library( kbibtexos SHARED ${onlinesearch_LIB_SRCS} )

target_link_libraries( kbibtexos
	${QT_QTCORE_LIBRARY}
	${QT_QTWEBKIT_LIBRARY}
	${KDE4_KDECORE_LIBS}
	${KDE4_KIO_LIBS}
	kbibtexio
)

install(TARGETS kbibtexos RUNTIME DESTINATION bin LIBRARY DESTINATION ${LIB_INSTALL_DIR})

