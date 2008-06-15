include_directories(
	${QT_INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}
)

set( libkbibtexio_SRCS
	comment.cpp
	element.cpp
	encoder.cpp
	encoderlatex.cpp
	encoderxml.cpp
	entry.cpp
	entryfield.cpp
	file.cpp
	fileexporter.cpp
	fileexporterbibtex.cpp
	fileimporter.cpp
	fileimporterbibtex.cpp
	macro.cpp
	preamble.cpp
	value.cpp
)

add_library( kbibtexio SHARED ${libkbibtexio_SRCS} )

target_link_libraries( kbibtexio
	${QT_QTCORE_LIBRARY}
)
