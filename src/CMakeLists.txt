include( CheckIncludeFiles )

include( KDE4Defaults )

# check if QtWebKit is available, which seems to be not available
# on e.g. RedHat Enterprise Linux 6 or Scientific Linux 6
IF(QT_QTWEBKIT_FOUND)
message(STATUS "Found QtWebKit, enabling WebKit support")
add_definitions(-DHAVE_QTWEBKIT)
ELSE(QT_QTWEBKIT_FOUND)
message(STATUS "QtWebKit not found, disabling WebKit support")
ENDIF(QT_QTWEBKIT_FOUND)


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
add_subdirectory( websearch )
add_subdirectory( gui )
add_subdirectory( program )
add_subdirectory( parts )

