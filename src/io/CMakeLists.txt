# KBibTeXIO library

set(
    kbibtexio_LIB_SRCS
    encoder.cpp
    encoderlatex.cpp
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
    textencoder.cpp
    bibutils.cpp
    xsltransform.cpp
    ${CMAKE_SOURCE_DIR}/src/global/kbibtex.cpp
    ${CMAKE_SOURCE_DIR}/src/global/preferences.cpp
    logging_io.cpp
)

set(
    kbibtexio_HDRS
    encoder.h
    encoderlatex.h
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
    textencoder.h
    bibutils.h
    xsltransform.h
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexio kbibtexio_LIB_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_BINARY_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/global
)

add_library(
    kbibtexio
    SHARED
    ${kbibtexio_LIB_SRCS}
)

target_link_libraries( kbibtexio
    Qt5::Core
    Qt5::Widgets
    Qt5::XmlPatterns
    Qt5::Concurrent
    KF5::I18n
    KF5::XmlGui
    Poppler::Qt5
    ${ICU_LIBRARIES}
    kbibtexconfig
    kbibtexdata
)

set_target_properties(
    kbibtexio
    PROPERTIES
    EXPORT_NAME "kbibtexio"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

install(
    TARGETS
    kbibtexio
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexio )
