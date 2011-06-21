# Processing library

set( processing_LIB_SRCS
	findduplicates.cpp
	lyx.cpp
)

add_definitions( -DMAKE_PROCESSING_LIB )

# debug area for KBibTeX's processing library
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101016)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../libkbibtexio
)

kde4_add_library( kbibtexproc SHARED ${processing_LIB_SRCS} )

target_link_libraries( kbibtexproc
	${QT_QTCORE_LIBRARY}
	${KDE4_KDECORE_LIBS}
	${KDE4_KDEUI_LIBS}
	kbibtexio
)

install(TARGETS kbibtexproc RUNTIME DESTINATION bin LIBRARY DESTINATION ${LIB_INSTALL_DIR})
