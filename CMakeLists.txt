project( kbibtex )

cmake_minimum_required(VERSION 2.6)

find_package( Qt4 REQUIRED )
find_package( KDE4 REQUIRED )

find_package(LibXslt)
macro_log_feature(LIBXSLT_FOUND "LibXSLT" "A library to transform XML files
into other XML files" "http://xmlsoft.org/XSLT" TRUE "" "Required to transform
XML files")

find_package(LibXml2)
macro_log_feature(LIBXML2_FOUND "LibXML2" "Libraries used to develop XML
applications" "http://xmlsoft.org" TRUE "" "Required to transform XML files")

add_subdirectory( src )
add_subdirectory( config )

