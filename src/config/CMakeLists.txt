# KBibTeXConfig library

set(
    kbibtexconfig_LIB_SRCS
    notificationhub.cpp
)

add_definitions(
    -DMAKE_KBIBTEXCONFIG_LIB
)

# debug area for KBibTeX's Config library
add_definitions(
    -DKDE_DEFAULT_DEBUG_AREA=101039
)

include_directories(
# TODO
)

if(UNITY_BUILD)
    enable_unity_build(kbibtexconfig kbibtexconfig_LIB_SRCS)
endif(UNITY_BUILD)

kde4_add_library(
    kbibtexconfig
    SHARED
    ${kbibtexconfig_LIB_SRCS}
)

target_link_libraries(
    kbibtexconfig
    LINK_PRIVATE
    ${QT_QTCORE_LIBRARY}
    ${KDE4_KDECORE_LIBS}
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
    RUNTIME
    DESTINATION
    bin
    LIBRARY
    DESTINATION
    ${LIB_INSTALL_DIR}
)
