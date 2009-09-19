# KBibTeXIO library

set( kbibtexio_LIB_SRCS
	comment.cpp
	element.cpp
	encoder.cpp
	encoderlatex.cpp
	encoderxml.cpp
	entry.cpp
	field.cpp
	file.cpp
	fileexporter.cpp
	fileexporterbibtex.cpp
	fileexporterpdf.cpp
	fileexporterps.cpp
	fileexporterris.cpp
	fileexporterrtf.cpp
	fileexportertoolchain.cpp
	fileexporterxml.cpp
	fileimporter.cpp
	fileimporterbibtex.cpp
	fileimporterris.cpp
	macro.cpp
	preamble.cpp
	value.cpp
)

add_definitions( -DMAKE_KBIBTEXIO_LIB )

kde4_add_library( kbibtexio SHARED ${kbibtexio_LIB_SRCS} )

target_link_libraries( kbibtexio
	${QT_QTCORE_LIBRARY}
)

install(TARGETS kbibtexio DESTINATION ${LIB_INSTALL_DIR})

