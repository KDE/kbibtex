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


# include(CheckIncludeFile)
include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)
# include(ECMInstallIcons)
# include(ECMGenerateHeaders)
include(GenerateExportHeader)
# include(ECMSetupVersion)
# include(ECMPackageConfigHelpers)
# include(FeatureSummary)

find_package(
    Qt5 ${QT_MIN_VERSION}
    CONFIG
    REQUIRED
    Core
    Widgets
    Network
)

find_package(
    KF5 ${KF5_MIN_VERSION}
    REQUIRED
    COMPONENTS
    I18n
    XmlGui
    KIO
)
#     Archive
#     Bookmarks
#     Config
#     ConfigWidgets
#     CoreAddons
#     DocTools
#     Emoticons
#     I18n
#     IdleTime
#     NotifyConfig
#     KIO
#     Parts
#     Solid
#     Sonnet
#     Wallet
#     WidgetsAddons
#     GlobalAccel
#     DBusAddons
#     Notifications
#     WindowSystem
#     IconThemes
#     ItemViews


# Version number for the exported headers / libs
set(
    LIB_VERSION "0.7.0"
)
set(
    LIB_SOVERSION "1"
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
# macro_log_feature(
#     LIBXSLT_FOUND
#     "LibXSLT"
#     "A library to transform XML files into other XML files"
#     "http://xmlsoft.org/XSLT"
#     TRUE
#     ""
#     "Required to transform XML files"
# )

find_package(
    LibXml2
    REQUIRED
)
# macro_log_feature(
#     LIBXML2_FOUND
#     "LibXML2"
#     "Libraries used to develop XML applications"
#     "http://xmlsoft.org"
#     TRUE
#     ""
#     "Required to transform XML files"
# )

# find_package(
#     PopplerQt5
#     REQUIRED
# )
# macro_log_feature(
#     PopplerQt4_FOUND
#     "Poppler/Qt4"
#     "Poppler is a PDF rendering library"
#     "http://poppler.freedesktop.org/"
#     TRUE
#     ""
#     "Required to transform process PDF files"
# )

# find_package(
#     Qxt
# )
# if(QXT_FOUND_QXTCORE AND QXT_FOUND_QXTGUI)
#     message(
#         STATUS
#         "Using system installation of LibQxt instead of built-in version"
#     )
# else()
#     message(
#         STATUS
#         "No system installation of LibQxt found, using built-in version instead"
#     )
# endif()

find_package(
    QCA2
    REQUIRED
)
find_package(
    QtOAuth
)
if(
    QTOAUTH_FOUND
)
    add_definitions(
        -DHAVE_QTOAUTH
    )
else(
    QTOAUTH_FOUND
)
    message(
        STATUS
        "No QtOAuth installed, disabling Zotero OAuth wizard"
    )
endif(
    QTOAUTH_FOUND
)


add_subdirectory(
    config
)
add_subdirectory(
    src
)
# add_subdirectory(
#     xslt
# )
# add_subdirectory(
#     icons
# )
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
