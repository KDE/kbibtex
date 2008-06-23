include_directories(
	${QT_INCLUDES}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	# ${BIBUTILSIncDir} # FIXME: Make inclusion depending on HAVE_BIBUTILS check
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
	fileexporterrtf.cpp
	fileexportertoolchain.cpp
	fileexporterxml.cpp
	fileimporter.cpp
	fileimporterbibtex.cpp
	# fileimporterbibutils.cpp # FIXME: Make inclusion depending on HAVE_BIBUTILS check
	fileimporterris.cpp
	macro.cpp
	preamble.cpp
	value.cpp
)

set( libkbibtexio_MOC_SRCS
	file.h
	fileexporterbibtex.h
	fileimporterbibtex.h
)

QT4_AUTOMOC( ${libkbibtexio_SRCS} )

add_library( kbibtexio SHARED ${libkbibtexio_SRCS} )

target_link_libraries( kbibtexio
	${QT_QTCORE_LIBRARY}
	# ${BIBUTILSLinkFlags} # FIXME: Make inclusion depending on HAVE_BIBUTILS check
	# ${BIBUTILSLinkDir} # FIXME: Make inclusion depending on HAVE_BIBUTILS check
)


# Test

set( kbibtexiotest_SRCS
	kbibtexiotest.cpp
)

add_executable( kbibtexiotest ${kbibtexiotest_SRCS} )

# target_link_libraries( kbibtexiotest kbibtexio )
target_link_libraries( kbibtexiotest
	${QT_QTCORE_LIBRARY}
	kbibtexio
)
