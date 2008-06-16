include_directories(
	${QT_INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

# KBibTeXIO library

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
	fileexporterpdf.cpp
	fileexporterps.cpp
	fileexporterris.cpp
	fileexportertoolchain.cpp
	fileexporterxml.cpp
	fileimporter.cpp
	fileimporterbibtex.cpp
	macro.cpp
	preamble.cpp
	value.cpp
)

set( libkbibtexio_MOC_SRCS
	file.h
	fileexporterbibtex.h
	fileimporterbibtex.h
)

QT4_AUTOMOC(${libkbibtexio_SRCS})

add_library( kbibtexio SHARED ${libkbibtexio_SRCS} )

target_link_libraries( kbibtexio
	${QT_QTCORE_LIBRARY}
)


# Test

set( kbibtexiotest_SRCS
	kbibtexiotest.cpp
)

add_executable(kbibtexiotest ${kbibtexiotest_SRCS})

target_link_libraries(kbibtexiotest kbibtexio)
