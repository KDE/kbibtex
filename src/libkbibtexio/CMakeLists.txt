# KBibTeXIO library

set( kbibtexio_LIB_SRCS
	comment.cpp
	element.cpp
	encoder.cpp
	encoderlatex.cpp
	encoderxml.cpp
	entry.cpp
	file.cpp
	fileexporter.cpp
	fileexporterbibtex.cpp
	fileexporterpdf.cpp
	fileexporterps.cpp
	fileexporterris.cpp
	fileexporterrtf.cpp
	fileexportertoolchain.cpp
	fileexporterbibtex2html.cpp
	fileexporterxml.cpp
	fileexporterxslt.cpp
	fileimporter.cpp
	fileimporterbibtex.cpp
	fileimporterris.cpp
	macro.cpp
	preamble.cpp
	value.cpp
)

add_definitions( -DMAKE_KBIBTEXIO_LIB )
include_directories( ${LIBXML2_INCLUDE_DIR} ${LIBXSLT_INCLUDE_DIR} )

kde4_add_library( kbibtexio SHARED ${kbibtexio_LIB_SRCS} )

target_link_libraries( kbibtexio
	${QT_QTCORE_LIBRARY}
	${KDE4_KDECORE_LIBS}
	${LIBXML2_LIBRARIES}
	${LIBXSLT_LIBRARIES}
)

install(TARGETS kbibtexio DESTINATION ${LIB_INSTALL_DIR})

