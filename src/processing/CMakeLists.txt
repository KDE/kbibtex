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
#     Qt5::Widgets
#     KF5::I18n
#     KF5::XmlGui
#     ${LIBXML2_LIBRARIES}
#     ${LIBXSLT_LIBRARIES}
    kbibtexconfig
    kbibtexdata
    kbibtexio
)

set_target_properties(
    kbibtexproc
    PROPERTIES
    EXPORT_NAME "kbibtexproc"
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexproc
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

install(
    FILES
    lyx.rc
    DESTINATION
    ${KXMLGUI_INSTALL_DIR}/kbibtex
)

generate_export_header( kbibtexproc )

# install(
#     FILES
#     ${kbibtexproc_HDRS}
#     ${CMAKE_BINARY_DIR}/src/processing/kbibtexproc_export.h
#     DESTINATION
#     ${INCLUDE_INSTALL_DIR}/kbibtex/proc
#     COMPONENT
#     development
# )
