include(
    CheckIncludeFiles
)

include(
    KDE4Defaults
)

##SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
##SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
##SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
##SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
##SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
##SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -pg")
##SET(CMAKE_CXX_FLAGS_DEBUGFULL "${CMAKE_CXX_FLAGS_DEBUGFULL} -pg")
##SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -pg")
##SET(CMAKE_CXX_FLAGS_DISTRIBUTION "${CMAKE_CXX_FLAGS_DISTRIBUTION} -pg")

# check if QtWebKit is available, which seems to be not available
# on e.g. RedHat Enterprise Linux 6 or Scientific Linux 6
if(
    QT_QTWEBKIT_FOUND
)
    message(
        STATUS
        "Found QtWebKit, enabling WebKit support"
    )
    add_definitions(
        -DHAVE_QTWEBKIT
    )
else(
    QT_QTWEBKIT_FOUND
)
    message(
        STATUS
        "QtWebKit not found, disabling WebKit support"
    )
endif(
    QT_QTWEBKIT_FOUND
)

if ( DEFINED ENV{SVN_REV} )
    message(
        STATUS
        "SVN version fetched from environment"
    )
    set(
        SVN_REVISION
        $ENV{SVN_REV}
    )
else( DEFINED ENV{SVN_REV} )
    mark_as_advanced(
        force
        SVNVERSION_EXECUTABLE
    )
    find_program(
        SVNVERSION_EXECUTABLE
        svnversion
    )
    if(
        SVNVERSION_EXECUTABLE
    )
        message(
            STATUS
            "Extracting SVN version, please wait..."
        )
        execute_process(
            COMMAND
            ${SVNVERSION_EXECUTABLE}
            -n
            ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE
            SVN_REVISION
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(
            SVN_REVISION
            MATCHES
            "exported"
        )
            message(
                STATUS
                "Not an SVN version, assuming non-developer release"
                "\nEnforce SVN revision number by setting environment variable SVN_REV"
            )
            set(
                SVN_REVISION
                "unknown"
            )
        endif(
            SVN_REVISION
            MATCHES
            "exported"
        )
    else(
        SVNVERSION_EXECUTABLE
    )
        message(
            STATUS
            "No Subversion installed, assuming non-developer release"
            "\nEnforce SVN revision number by setting environment variable SVN_REV"
        )
        set(
            SVN_REVISION
            "unknown"
        )
    endif(
        SVNVERSION_EXECUTABLE
    )
endif( DEFINED ENV{SVN_REV} )

message(
    STATUS
    "SVN version is "
    ${SVN_REVISION}
)
add_custom_target(
    set_version_h_in_builddir
    ALL
    ${CMAKE_COMMAND}
    -E
    echo
    "#ifndef VERSION_H"
    >
    "${CMAKE_BINARY_DIR}/src/version.h.tmp"
    COMMAND
    ${CMAKE_COMMAND}
    -E
    echo
    "#define VERSION_H"
    >>
    "${CMAKE_BINARY_DIR}/src/version.h.tmp"
    COMMAND
    ${CMAKE_COMMAND}
    -E
    echo
    "const char *versionNumber = \"SVN r${SVN_REVISION}\";"
    >>
    "${CMAKE_BINARY_DIR}/src/version.h.tmp"
    COMMAND
    ${CMAKE_COMMAND}
    -E
    echo
    "#endif // VERSION_H"
    >>
    ${CMAKE_BINARY_DIR}/src/version.h.tmp
    COMMAND
    ${CMAKE_COMMAND}
    -E
    copy_if_different
    "${CMAKE_BINARY_DIR}/src/version.h.tmp"
    "${CMAKE_BINARY_DIR}/src/version.h"
    COMMAND
    ${CMAKE_COMMAND}
    -E
    remove
    "${CMAKE_BINARY_DIR}/src/version.h.tmp"
    WORKING_DIRECTORY
    "${CMAKE_SOURCE_DIR}"
    COMMENT
    "Updating ${CMAKE_BINARY_DIR}/src/version.h..."
    VERBATIM
)
set_source_files_properties(${CMAKE_BINARY_DIR}/src/version.h
    PROPERTIES GENERATED TRUE
    HEADER_FILE_ONLY TRUE)


add_definitions(
    ${QT_DEFINITIONS}
    ${KDE4_DEFINITIONS}
)
include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/networking
    ${KDE4_INCLUDES}
)

add_subdirectory(
    libkbibtexio
)
add_subdirectory(
    processing
)
add_subdirectory(
    networking
)
add_subdirectory(
    gui
)
add_subdirectory(
    program
)
add_subdirectory(
    parts
)

install(
    FILES
    kbibtexnamespace.h
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/kbibtex
    COMPONENT
    Devel
)
