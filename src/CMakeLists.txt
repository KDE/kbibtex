include( CheckIncludeFiles )

check_include_files( bibutils.h HAVE_BIBUTILS_H )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h )

if(NOT WIN32)
	option( WITH_BIBUTILS "Enable bibutils library support" ON )
	include( UsePkgConfig )

	if ( WITH_BIBUTILS )
		pkgconfig( bibutils BIBUTILSIncDir BIBUTILSLinkDir BIBUTILSLinkFlags BIBUTILSCflags)
  		if ( BIBUTILSCflags )
			set( HAVE_BIBUTILS 1 )
		endif( BIBUTILSCflags )
	endif( WITH_BIBUTILS )
endif( NOT WIN32)

add_definitions( ${BIBUTILSCflags} )

add_subdirectory( libkbibtexio )
