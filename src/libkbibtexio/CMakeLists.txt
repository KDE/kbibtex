# KBibTeXIO library

set( kbibtexio_LIB_SRCS
	xsltransform.cpp
	comment.cpp
	element.cpp
	encoder.cpp
	encoderlatex.cpp
	encoderxml.cpp
	entry.cpp
	file.cpp
	fileinfo.cpp
	fileexporter.cpp
	fileexporterbibtex.cpp
	fileexporterblg.cpp
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
	fileimporterpdf.cpp
	macro.cpp
	preamble.cpp
	iconvlatex.cpp
	value.cpp
	config/bibtexfields.cpp
	config/bibtexentries.cpp
)

add_definitions( -DMAKE_KBIBTEXIO_LIB )

# debug area for KBibTeX's IO library unless something else is specified
add_definitions(-DKDE_DEFAULT_DEBUG_AREA=101011)

include_directories(
	${LIBXML2_INCLUDE_DIR}
	${LIBXSLT_INCLUDE_DIR}
	${POPPLER_QT4_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/config/
)

kde4_add_library( kbibtexio SHARED ${kbibtexio_LIB_SRCS} )

target_link_libraries( kbibtexio
	${QT_QTCORE_LIBRARY}
	${KDE4_KDECORE_LIBS}
	${KDE4_KIO_LIBS}
	${LIBXML2_LIBRARIES}
	${LIBXSLT_LIBRARIES}
	${POPPLER_QT4_LIBRARIES}
)

install(TARGETS kbibtexio DESTINATION ${LIB_INSTALL_DIR})

