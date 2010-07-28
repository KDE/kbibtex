include( CheckIncludeFiles )

# find_package( Automoc4 REQUIRED )
find_package( KDE4 4.1.90 REQUIRED )
include( KDE4Defaults )

MARK_AS_ADVANCED(SVNVERSION_EXECUTABLE)
FIND_PROGRAM(SVNVERSION_EXECUTABLE svnversion)
IF(SVNVERSION_EXECUTABLE)
	ADD_CUSTOM_TARGET(svnversion ALL
		cmake -E echo "#ifndef VERSION_H" > "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "#define VERSION_H" >> "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo_append "const char *versionNumber = \"SVN r" >> "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND svnversion -n ${CMAKE_CURRENT_SOURCE_DIR} >> "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "\";" >>  "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "#endif // VERSION_H" >>  "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E copy_if_different "${CMAKE_BINARY_DIR}/version.h.tmp" "${CMAKE_BINARY_DIR}/version.h"
		COMMAND cmake -E remove "${CMAKE_BINARY_DIR}/version.h.tmp"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Updating ${CMAKE_BINARY_DIR}/version.h..."
		VERBATIM)
ELSE(SVNVERSION_EXECUTABLE)
	ADD_CUSTOM_TARGET(svnversion ALL
		cmake -E echo "#ifndef VERSION_H" > "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "#define VERSION_H" >> "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "const char *versionNumber = \"0.3\";" >> "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "#endif // VERSION_H" >>  "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E copy_if_different "${CMAKE_BINARY_DIR}/version.h.tmp" "${CMAKE_BINARY_DIR}/version.h"
		COMMAND cmake -E remove "${CMAKE_BINARY_DIR}/version.h.tmp"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Updating ${CMAKE_BINARY_DIR}/version.h..."
		VERBATIM)
ENDIF(SVNVERSION_EXECUTABLE)

if (NOT SVNVERSION OR SVNVERSION STREQUAL exported)
else (SVNVERSION)
	add_definitions(-DSVNREVISION=" \(SVN r""${SVNVERSION}""\)")
endif (NOT SVNVERSION OR SVNVERSION STREQUAL exported)

add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )

add_definitions(
	${QT_DEFINITIONS}
	${KDE4_DEFINITIONS}
)
include_directories(
	${PROJECT_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}
	${CMAKE_BINARY_DIR}
	${KDE4_INCLUDES}
)

add_subdirectory( libkbibtexio )
add_subdirectory( websearch )
add_subdirectory( gui )
add_subdirectory( program )
add_subdirectory( parts )

