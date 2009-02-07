include( CheckIncludeFiles )

# find_package( Automoc4 REQUIRED )
find_package( KDE4 4.1.90 REQUIRED )
include( KDE4Defaults )

add_definitions( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )

add_subdirectory( libkbibtexio )
add_subdirectory( gui )
