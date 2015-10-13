# KBibTeXConfig library

set(
    kbibtexconfig_LIB_SRCS
    notificationhub.cpp
    bibtexentries.cpp
    bibtexfields.cpp
    logging_config.cpp
)

set(
    kbibtexconfig_HDRS
    notificationhub.h
    bibtexentries.h
    bibtexfields.h
)

# # debug area for KBibTeX's Config library
# add_definitions(
#     -DKDE_DEFAULT_DEBUG_AREA=101039
# )

if(UNITY_BUILD)
    enable_unity_build(kbibtexconfig kbibtexconfig_LIB_SRCS)
endif(UNITY_BUILD)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

add_library(
    kbibtexconfig
    SHARED
    ${kbibtexconfig_LIB_SRCS}
)

target_link_libraries( kbibtexconfig
    Qt5::Core
    KF5::I18n
    KF5::XmlGui
)

set_target_properties(
    kbibtexconfig
    PROPERTIES
    VERSION
    ${LIB_VERSION}
    SOVERSION
    ${LIB_SOVERSION}
)

install(
    TARGETS
    kbibtexconfig
    ${INSTALL_TARGETS_DEFAULT_ARGS}
)

generate_export_header( kbibtexconfig )

# install(
#     FILES
#     ${kbibtexconfig_HDRS}
#     ${CMAKE_BINARY_DIR}/src/data/kbibtexdata_export.h
#     DESTINATION
#     ${INCLUDE_INSTALL_DIR}/kbibtex/config
#     COMPONENT
#     development
# )
