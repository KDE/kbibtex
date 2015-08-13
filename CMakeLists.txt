project(
    kbibtex
)

cmake_minimum_required(
    VERSION
    2.8.12
    FATAL_ERROR
)

set(
    QT_MIN_VERSION "5.3.0"
)
set(
    KF5_MIN_VERSION "5.6.0"
)

find_package(ECM 1.7.0 REQUIRED NO_MODULE)

add_definitions(-DTRANSLATION_DOMAIN="kbibtex")

set(
    CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    /usr/share/apps/cmake/modules/
    ${ECM_MODULE_PATH}
    ${ECM_KDE_MODULE_DIR}
    ${CMAKE_SOURCE_DIR}/cmake/modules
)


include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(GenerateExportHeader)

find_package(
    Qt5 ${QT_MIN_VERSION}
    CONFIG
    REQUIRED
    Core
    Widgets
    Network
)

#FIXME this code fragment is intended to test for optional
# components (WebEngineWidgets and WebKitWidgets) but fails
# with warnings about missing 'Qt5' if either component
# is not installed.
#find_package(
#    Qt5 ${QT_MIN_VERSION}
#    QUIET
#    CONFIG
#    COMPONENTS
#    WebEngineWidgets
#    WebKitWidgets
#)

find_package(
    KF5 ${KF5_MIN_VERSION}
    REQUIRED
    I18n
    XmlGui
    KIO
    IconThemes
    ItemViews
    Completion
    Parts
    CoreAddons
    Service
    KDELibs4Support
)

# Version number for the exported headers / libs
set(
    VERSION_BRANCH "0.8"
)
set(
    REVISION_VERSION "0"
)
set(
    LIB_VERSION ${VERSION_BRANCH}.${REVISION_VERSION}
)
set(
    LIB_SOVERSION ${VERSION_BRANCH}
)

# option(
#     UNITY_BUILD
#     "Compile multiple C++ files in one big, merged file (\"Unity build\")\nSee also http://t-fischer.dreamwidth.org/3054.html"
# )
# if(UNITY_BUILD)
#     message(STATUS "Unity build enabled")
# else(UNITY_BUILD)
#     message(STATUS "Unity build disabled (default), use option UNITY_BUILD to enable it")
# endif(UNITY_BUILD)

# # FIXME may have to be cleaned up a little bit
# # Contributed by Jeremy Cribb <cribb.jeremy@gmail.com>
# if(
#     APPLE
# )
#     find_library(
#         SYS_CONFIG_LIBRARY
#         SystemConfiguration
#     )
#     mark_as_advanced(
#         SYS_CONFIG_LIBRARY
#     )
#     # SET(TARGET_EXTERNAL_LIBRARIES iconv ${SYS_CONFIG_LIBRARY})
#     set(
#         ICONV_INCLUDE_DIR
#         "/opt/local/include"
#     )
#     set(
#         ICONV_LIBRARIES
#         "/opt/local/lib/libiconv.dylib"
#     )
#     set(
#         LIBXSLT_LIBRARIES
#         "/opt/local/lib/libxslt.dylib"
#     )
# endif(
#     APPLE
# )

find_package(
    LibXslt
    REQUIRED
)

find_package(
    LibXml2
    REQUIRED
)

find_package(
    Poppler
    REQUIRED
)

find_package(
    ICU
    REQUIRED
    COMPONENTS uc i18n
)
if(
    ICU_FOUND
)
    message(
        STATUS
        "Found ICU " ${ICU_VERSION}
    )
    include_directories(${ICU_INCLUDE_DIRS})
endif(
    ICU_FOUND
)


add_subdirectory(
    config
)
add_subdirectory(
    src
)
add_subdirectory(
    xslt
)
# add_subdirectory(
#     doc
# )
# add_subdirectory(
#     mime
# )
# macro_optional_add_subdirectory(
#     po
# )

# install(FILES kbibtex.appdata.xml DESTINATION ${SHARE_INSTALL_PREFIX}/appdata/)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
