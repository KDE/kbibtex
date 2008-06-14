include_directories(
	${QT_INCLUDES}
)

set( libkbibtexio_SRCS
	value.cpp
)

add_library( kbibtexio SHARED ${libkbibtexio_SRCS} )

target_link_libraries( kbibtexio
	${QT_QTCORE_LIBRARY}
)
