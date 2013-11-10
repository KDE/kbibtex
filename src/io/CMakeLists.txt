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
    # FIXME fileimporterbibutils.cpp
    fileimporter.cpp
    fileimporterpdf.cpp
    fileimporterris.cpp
    fileinfo.cpp
    iconvlatex.cpp
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
    # FIXME fileimporterbibutils.h
    fileimporter.h
    fileimporterpdf.h
    fileimporterris.h
    fileinfo.h
    iconvlatex.h
    iocommon.h
    kbibtexio_export.h
    xsltransform.h
)

add_definitions(
    -DMAKE_KBIBTEXIO_LIB
)

# debug area for KBibTeX's IO library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101011
)

include_directories(
    ${LIBXML2_INCLUDE_DIR}
    ${LIBXSLT_INCLUDE_DIR}
    ${POPPLER_QT4_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexio kbibtexio_LIB_SRCS)
endif(UNITY_BUILD)

kde4_add_library(
    kbibtexio
    SHARED
    ${kbibtexio_LIB_SRCS}
)

if(
    WIN32
)
    target_link_libraries(
        kbibtexio
        ${QT_QTCORE_LIBRARY}
        ${KDE4_KDECORE_LIBS}
        ${KDE4_KIO_LIBS}
        ${LIBXML2_LIBRARIES}
        ${LIBXSLT_LIBRARIES}
        ${POPPLER_QT4_LIBRARIES}
        iconv
        kbibtexconfig
        kbibtexdata
    )
else(
    WIN32
)
    target_link_libraries(
        kbibtexio
        ${QT_QTCORE_LIBRARY}
        ${KDE4_KDECORE_LIBS}
        ${KDE4_KIO_LIBS}
        ${LIBXML2_LIBRARIES}
        ${LIBXSLT_LIBRARIES}
        ${POPPLER_QT4_LIBRARIES}
        kbibtexconfig
        kbibtexdata
    )
endif(
    WIN32
)

set_target_properties(
    kbibtexio
    PROPERTIES
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexio
    RUNTIME
    DESTINATION
    bin
    LIBRARY
    DESTINATION
    ${LIB_INSTALL_DIR}
)

install(
    FILES
    ${kbibtexio_HDRS}
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex
    COMPONENT
    Devel
)
