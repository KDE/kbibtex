# KBibTeX Processing library

set(
    kbibtexproc_LIB_SRCS
    findduplicates.cpp
    idsuggestions.cpp
    lyx.cpp
    checkbibtex.cpp
    bibliographyservice.cpp
    journalabbreviations.cpp
    logging_processing.cpp
)

set(
    kbibtexproc_HDRS
    findduplicates.h
    idsuggestions.h
    lyx.h
    checkbibtex.h
    bibliographyservice.h
    journalabbreviations.h
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexproc kbibtexproc_LIB_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/data
    ${CMAKE_SOURCE_DIR}/src/data
    ${CMAKE_BINARY_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_BINARY_DIR}/src/gui
    ${CMAKE_SOURCE_DIR}/src/gui
)

add_library(
    kbibtexproc
    SHARED
    ${kbibtexproc_LIB_SRCS}
)

target_link_libraries( kbibtexproc
    Qt5::Core
    KF5::Parts
    kbibtexconfig
    kbibtexdata
    kbibtexio
)

set_target_properties(
    kbibtexproc
    PROPERTIES
    EXPORT_NAME "kbibtexproc"
    VERSION ${KBIBTEX_RELEASE_VERSION}
    SOVERSION ${KBIBTEX_SOVERSION}
)

install(
    TARGETS
    kbibtexproc
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexproc )
