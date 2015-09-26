# KBibTeXData library

set(
    kbibtexdata_LIB_SRCS
    comment.cpp
    element.cpp
    entry.cpp
    file.cpp
    macro.cpp
    preamble.cpp
    value.cpp
    models/filemodel.cpp
    logging_data.cpp
)

set(
    kbibtexdata_HDRS
    comment.h
    element.h
    entry.h
    file.h
    macro.h
    preamble.h
    value.h
    models/filemodel.h
)

# # debug area for KBibTeX's Data library
# add_definitions(
#     -DKDE_DEFAULT_DEBUG_AREA=101032
# )

if(UNITY_BUILD)
    enable_unity_build(kbibtexdata kbibtexdata_LIB_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/config
    ${CMAKE_BINARY_DIR}/src/config
    ${CMAKE_SOURCE_DIR}/src
)

add_library(
    kbibtexdata
    SHARED
    ${kbibtexdata_LIB_SRCS}
)

target_link_libraries( kbibtexdata
    Qt5::Core
    Qt5::Widgets
    KF5::I18n
    KF5::XmlGui
    kbibtexconfig
)

# if(
#     WIN32
# )
#     target_link_libraries(
#         kbibtexdata
#         LINK_PRIVATE
#         iconv
#     )
# endif(
#     WIN32
# )

set_target_properties(
    kbibtexdata
    PROPERTIES
    EXPORT_NAME "kbibtexdata"
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexdata
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexdata )

install(
    FILES
    ${kbibtexdata_HDRS}
    ${CMAKE_BINARY_DIR}/src/data/kbibtexdata_export.h
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex/data
    COMPONENT
    development
)
