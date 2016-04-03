project(
    kbibtex
)

cmake_minimum_required(
    VERSION
    2.6
)

# Force various old and deprecated behaviors from CMake's 2.6 era
# in modern CMake 3.3 and later
if(POLICY CMP0002)
    # see also: https://cmake.org/cmake/help/v3.3/policy/CMP0002.html
    cmake_policy(SET CMP0002 OLD)
endif()
if(POLICY CMP0013)
    # see also: https://cmake.org/cmake/help/v3.3/policy/CMP0013.html
    cmake_policy(SET CMP0013 OLD)
endif()
if(POLICY CMP0022)
    # see also: https://cmake.org/cmake/help/v3.3/policy/CMP0022.html
    cmake_policy(SET CMP0022 OLD)
endif()
if(POLICY CMP0059)
    # see also: https://cmake.org/cmake/help/v3.3/policy/CMP0059.html
    cmake_policy(SET CMP0059 OLD)
endif()


set(
    CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake/modules
    ${CMAKE_MODULE_PATH}
)

# Version number for the exported headers / libs
set(
    VERSION_BRANCH "0.6"
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

option(
    UNITY_BUILD
    "Compile multiple C++ files in one big, merged file (\"Unity build\")\nSee also http://t-fischer.dreamwidth.org/3054.html"
)
if(UNITY_BUILD)
    message(STATUS "Unity build enabled")
else(UNITY_BUILD)
    message(STATUS "Unity build disabled (default), use option UNITY_BUILD to enable it")
endif(UNITY_BUILD)

find_package(
    Qt4
    REQUIRED
)
find_package(
    KDE4
    REQUIRED
)
include(
    KDE4Defaults
)

# FIXME may have to be cleaned up a little bit
# Contributed by Jeremy Cribb <cribb.jeremy@gmail.com>
if(
    APPLE
)
    find_library(
        SYS_CONFIG_LIBRARY
        SystemConfiguration
    )
    mark_as_advanced(
        SYS_CONFIG_LIBRARY
    )
    # SET(TARGET_EXTERNAL_LIBRARIES iconv ${SYS_CONFIG_LIBRARY})
    set(
        ICONV_INCLUDE_DIR
        "/opt/local/include"
    )
    set(
        ICONV_LIBRARIES
        "/opt/local/lib/libiconv.dylib"
    )
    set(
        LIBXSLT_LIBRARIES
        "/opt/local/lib/libxslt.dylib"
    )
endif(
    APPLE
)

find_package(
    LibXslt
    REQUIRED
)
macro_log_feature(
    LIBXSLT_FOUND
    "LibXSLT"
    "A library to transform XML files into other XML files"
    "http://xmlsoft.org/XSLT"
    TRUE
    ""
    "Required to transform XML files"
)

find_package(
    LibXml2
    REQUIRED
)
macro_log_feature(
    LIBXML2_FOUND
    "LibXML2"
    "Libraries used to develop XML applications"
    "http://xmlsoft.org"
    TRUE
    ""
    "Required to transform XML files"
)

find_package(
    PopplerQt4
    REQUIRED
)
macro_log_feature(
    PopplerQt4_FOUND
    "Poppler/Qt4"
    "Poppler is a PDF rendering library"
    "http://poppler.freedesktop.org/"
    TRUE
    ""
    "Required to transform process PDF files"
)

find_package(
    Qxt
)
if(QXT_FOUND_QXTCORE AND QXT_FOUND_QXTGUI)
    message(
        STATUS
        "Using system installation of LibQxt instead of built-in version"
    )
else()
    message(
        STATUS
        "No system installation of LibQxt found, using built-in version instead"
    )
endif()

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
    src
)
add_subdirectory(
    config
)
add_subdirectory(
    xslt
)
add_subdirectory(
    icons
)
add_subdirectory(
    doc
)
add_subdirectory(
    mime
)
macro_optional_add_subdirectory(
    po
)

install(FILES kbibtex.appdata.xml DESTINATION ${SHARE_INSTALL_PREFIX}/appdata/)
