# KBibTeXIO library

set(
    kbibtexio_LIB_SRCS
    config/bibtexentries.cpp
    config/bibtexfields.cpp
    encoder.cpp
    encoderlatex.cpp
    encoderutf8.cpp
    encoderxml.cpp
    fileexporterbibtex2html.cpp
    fileexporterbibtex.cpp
    fileexporterbibutils.cpp
    fileexporterbibtexoutput.cpp
    fileexporter.cpp
    fileexporterpdf.cpp
    fileexporterps.cpp
    fileexporterris.cpp
    fileexporterrtf.cpp
    fileexportertoolchain.cpp
    fileexporterxml.cpp
    fileexporterxslt.cpp
    fileimporterbibtex.cpp
    fileimporterbibutils.cpp
    fileimporter.cpp
    fileimporterpdf.cpp
    fileimporterris.cpp
    fileinfo.cpp
    iconvlatex.cpp
    bibutils.cpp
    xsltransform.cpp
)

set(
    kbibtexio_HDRS
    config/bibtexentries.h
    config/bibtexfields.h
    encoder.h
    encoderlatex.h
    encoderutf8.h
    encoderxml.h
    fileexporterbibtex2html.h
    fileexporterbibtex.h
    fileexporterbibutils.h
    fileexporterbibtexoutput.h
    fileexporter.h
    fileexporterpdf.h
    fileexporterps.h
    fileexporterris.h
    fileexporterrtf.h
    fileexportertoolchain.h
    fileexporterxml.h
    fileexporterxslt.h
    fileimporterbibtex.h
    fileimporterbibutils.h
    fileimporter.h
    fileimporterpdf.h
    fileimporterris.h
    fileinfo.h
    iconvlatex.h
    iocommon.h
    bibutils.h
    xsltransform.h
)

# add_definitions(
#     -DMAKE_KBIBTEXIO_LIB
# )

# # debug area for KBibTeX's IO library
# add_definitions(
#     -DKDE_DEFAULT_DEBUG_AREA=101011
# )

# if(UNITY_BUILD)
#     enable_unity_build(kbibtexio kbibtexio_LIB_SRCS)
# endif(UNITY_BUILD)

include_directories(
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
#     ${POPPLER_QT5_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_BINARY_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src
)

add_library(
    kbibtexio
    SHARED
    ${kbibtexio_LIB_SRCS}
)

target_link_libraries( kbibtexio
    Qt5::Core
    Qt5::Widgets
    KF5::I18n
    KF5::XmlGui
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    kbibtexconfig
    kbibtexdata
)

# if(
#     WIN32
# )
#     target_link_libraries(
#         kbibtexio
#         LINK_PRIVATE
#         iconv
#     )
# endif(
#     WIN32
# )

set_target_properties(
    kbibtexio
    PROPERTIES
    EXPORT_NAME "kbibtexio"
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexio
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexio )

install(
    FILES
    ${kbibtexio_HDRS}
    ${CMAKE_BINARY_DIR}/src/io/kbibtexio_export.h
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex/io
    COMPONENT
    development
)
