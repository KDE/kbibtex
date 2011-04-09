project( kbibtex )

cmake_minimum_required(VERSION 2.6)

set(MANDIR "share/man" CACHE STRING "Where to install manpages")

find_package( Qt4 REQUIRED )
find_package( KDE4 REQUIRED )

# FIXME may have to be cleaned up a little bit
# Contributed by Jeremy Cribb <cribb.jeremy@gmail.com>
IF(APPLE)
  FIND_LIBRARY(SYS_CONFIG_LIBRARY SystemConfiguration )
  MARK_AS_ADVANCED(SYS_CONFIG_LIBRARY)
# SET(TARGET_EXTERNAL_LIBRARIES iconv ${SYS_CONFIG_LIBRARY})
   SET(ICONV_INCLUDE_DIR   "/opt/local/include")
   SET(ICONV_LIBRARIES     "/opt/local/lib/libiconv.dylib")
   SET(LIBXSLT_LIBRARIES   "/opt/local/lib/libxslt.dylib")
ENDIF(APPLE)

find_package(LibXslt)
macro_log_feature(LIBXSLT_FOUND "LibXSLT" "A library to transform XML files
into other XML files" "http://xmlsoft.org/XSLT" TRUE "" "Required to transform
XML files")

find_package(LibXml2)
macro_log_feature(LIBXML2_FOUND "LibXML2" "Libraries used to develop XML
applications" "http://xmlsoft.org" TRUE "" "Required to transform XML files")

find_package(PopplerQt4)

add_subdirectory( src )
add_subdirectory( config )
add_subdirectory( xslt )
add_subdirectory( icons )
add_subdirectory( man )
add_subdirectory( mime )
