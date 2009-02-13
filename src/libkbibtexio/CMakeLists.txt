# KBibTeXIO library

set( kbibtexio_LIB_SRCS
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
	# ${BIBUTILSLinkFlags} # FIXME: Make inclusion depending on HAVE_BIBUTILS check
	# ${BIBUTILSLinkDir} # FIXME: Make inclusion depending on HAVE_BIBUTILS check
)


# Test

set( kbibtexiotest_SRCS
	kbibtexiotest.cpp
)

automoc4_add_executable( kbibtexiotest ${kbibtexiotest_SRCS} )

target_link_libraries( kbibtexiotest
	${QT_QTCORE_LIBRARY}
	kbibtexio
)
