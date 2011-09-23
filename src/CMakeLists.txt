include( CheckIncludeFiles )

include( KDE4Defaults )

##SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
##SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
##SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
##SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
##SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
##SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -pg")
##SET(CMAKE_CXX_FLAGS_DEBUGFULL "${CMAKE_CXX_FLAGS_DEBUGFULL} -pg")
##SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -pg")
##SET(CMAKE_CXX_FLAGS_DISTRIBUTION "${CMAKE_CXX_FLAGS_DISTRIBUTION} -pg")


# check if QtWebKit is available, which seems to be not available
# on e.g. RedHat Enterprise Linux 6 or Scientific Linux 6
IF(QT_QTWEBKIT_FOUND)
message(STATUS "Found QtWebKit, enabling WebKit support")
add_definitions(-DHAVE_QTWEBKIT)
ELSE(QT_QTWEBKIT_FOUND)
message(STATUS "QtWebKit not found, disabling WebKit support")
ENDIF(QT_QTWEBKIT_FOUND)


MARK_AS_ADVANCED(SVNVERSION_EXECUTABLE)
FIND_PROGRAM(SVNVERSION_EXECUTABLE svnversion)
IF(SVNVERSION_EXECUTABLE)
IF(WIN32)
# single \" in an echo statement makes problems under Windows; don't know why
	ADD_CUSTOM_TARGET(svnversion ALL
		cmake -E echo "#ifndef VERSION_H" > "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "#define VERSION_H" >> "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "const char *versionNumber = \"SVN unknown\";" >>  "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E echo "#endif // VERSION_H" >>  "${CMAKE_BINARY_DIR}/version.h.tmp"
		COMMAND cmake -E copy_if_different "${CMAKE_BINARY_DIR}/version.h.tmp" "${CMAKE_BINARY_DIR}/version.h"
		COMMAND cmake -E remove "${CMAKE_BINARY_DIR}/version.h.tmp"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
		COMMENT "Updating ${CMAKE_BINARY_DIR}/version.h..."
		VERBATIM)
ELSE(WIN32)
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
ENDIF(WIN32)
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
add_subdirectory( processing )
add_subdirectory( networking )
add_subdirectory( gui )
add_subdirectory( program )
add_subdirectory( parts )

