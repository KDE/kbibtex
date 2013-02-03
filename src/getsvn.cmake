# Inspired by:
# http://stackoverflow.com/questions/3780667/use-cmake-to-get-build-time-svn-revision

if(
    DEFINED
    ENV{SVN_REV}
)
    message(
        STATUS
        "SVN version was set outside through environment variable SVN_REV"
    )
    set(
        SVN_REVISION
        $ENV{SVN_REV}
    )
endif(
    DEFINED
    ENV{SVN_REV}
)

if(
    NOT
    DEFINED
    SVN_REVISION
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
            "Extracting SVN version ..."
        )
        execute_process(
            COMMAND
            ${SVNVERSION_EXECUTABLE}
            -n
            ${SOURCE_DIR}
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
        )
        set(
            SVN_REVISION
            "unknown"
        )
    endif(
        SVNVERSION_EXECUTABLE
    )
endif(
    NOT
    DEFINED
    SVN_REVISION
)

# write a file with the SVNVERSION define
file(
    WRITE
    "${BINARY_DIR}/src/version.h.tmp"
    "#ifndef VERSION_H\n"
)
file(
    APPEND
    "${BINARY_DIR}/src/version.h.tmp"
    "#define VERSION_H\n"
)
file(
    APPEND
    "${BINARY_DIR}/src/version.h.tmp"
    "const char *versionNumber = \"SVN revision ${SVN_REVISION}\";\n"
)
file(
    APPEND
    "${BINARY_DIR}/src/version.h.tmp"
    "#endif // VERSION_H\n"
)

message(
    STATUS
    "SVN version is "
    ${SVN_REVISION}
)

if(
    EXISTS
    "${BINARY_DIR}/src/version.h.tmp"
)
    execute_process(
        COMMAND
        ${CMAKE_COMMAND}
        -E
        copy_if_different
        "${BINARY_DIR}/src/version.h.tmp"
        "${BINARY_DIR}/src/version.h"
        WORKING_DIRECTORY
        "${SOURCE_DIR}"
    )
    execute_process(
        COMMAND
        ${CMAKE_COMMAND}
        -E
        remove
        "${BINARY_DIR}/src/version.h.tmp"
        WORKING_DIRECTORY
        "${SOURCE_DIR}"
    )
else(
    EXISTS
    "${BINARY_DIR}/src/version.h.tmp"
)
    message(
        STATUS
        "${BINARY_DIR}/src/version.h.tmp does not exist"
    )
endif(
    EXISTS
    "${BINARY_DIR}/src/version.h.tmp"
)
