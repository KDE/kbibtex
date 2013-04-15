# Inspired by:
# http://stackoverflow.com/questions/3780667/use-cmake-to-get-build-time-svn-revision

if (DEFINED ENV{SVN_REV})
    message (STATUS "SVN version set by environment variable SVN_REV")
    set (SVN_REVISION $ENV{SVN_REV})
    if (DEFINED ENV{SVN_PATH})
        message (STATUS "SVN path set by environment variable SVN_PATH")
	set (SVN_OUTPUT $ENV{SVN_PATH}) # set SVN_OUTPUT here, will be processed to SVN_PATH later
    endif (DEFINED ENV{SVN_PATH})
else (DEFINED ENV{SVN_REV})
    set(SVN_REVISION "unknown")
    if (EXISTS ${SOURCE_DIR}/.svn)
        set(SCM "svn")
    elseif (EXISTS ${SOURCE_DIR}/.git)
        set(SCM "git")
    else (EXISTS ${SOURCE_DIR}/.svn)
        message(STATUS "No SCM directory found. Assuming non-developer release.")
    endif (EXISTS ${SOURCE_DIR}/.svn)
    
    
    # SVN
    if (SCM MATCHES "svn" )
        find_program (SVN_EXECUTABLE NAMES svn svn.exe svn.bat)
        find_program (SVNVERSION_EXECUTABLE NAMES svnversion svnversion.exe svnversion.bat)
        if (SVN_EXECUTABLE AND SVNVERSION_EXECUTABLE )
            execute_process (COMMAND ${SVNVERSION_EXECUTABLE} -n ${SOURCE_DIR}
                             OUTPUT_VARIABLE SVN_REVISION
                             OUTPUT_STRIP_TRAILING_WHITESPACE)
            execute_process (COMMAND ${SVN_EXECUTABLE} info ${SOURCE_DIR}
                             OUTPUT_VARIABLE SVN_OUTPUT
                             OUTPUT_STRIP_TRAILING_WHITESPACE)
        else (SVN_EXECUTABLE AND SVNVERSION_EXECUTABLE )
            message( "SVN configuration found, but no SVN executable." )
        endif (SVN_EXECUTABLE AND SVNVERSION_EXECUTABLE )
    endif (SCM MATCHES "svn" )
    
    
    # Git
    if (SCM MATCHES "git" )
        find_program( GIT_EXECUTABLE NAMES git )  # FIXME git.exe? -> not tested on windows
        if (GIT_EXECUTABLE)
            execute_process (COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
                             OUTPUT_VARIABLE GIT_REV_TREEISH
                             OUTPUT_STRIP_TRAILING_WHITESPACE)
            execute_process (COMMAND ${GIT_EXECUTABLE} svn find-rev ${GIT_REV_TREEISH}
                             OUTPUT_VARIABLE SVN_REVISION
                             OUTPUT_STRIP_TRAILING_WHITESPACE)
            execute_process (COMMAND ${GIT_EXECUTABLE} svn info
                             OUTPUT_VARIABLE SVN_OUTPUT
                             OUTPUT_STRIP_TRAILING_WHITESPACE)
        else (GIT_EXECUTABLE)
            message( "Git configuration found, but no git executable." )
        endif (GIT_EXECUTABLE)
    endif( SCM MATCHES "git" )
endif (DEFINED ENV{SVN_REV})

if (DEFINED SVN_OUTPUT)
    string( REGEX MATCH "(trunk|branches|tags)(/.*)?" SVN_PATH ${SVN_OUTPUT})
endif (DEFINED SVN_OUTPUT)

# write a file with the SVNVERSION define
file(
    WRITE
    "${BINARY_DIR}/version.h.tmp"
    "#ifndef VERSION_H\n"
)
file(
    APPEND
    "${BINARY_DIR}/version.h.tmp"
    "#define VERSION_H\n"
)
file(
    APPEND
    "${BINARY_DIR}/version.h.tmp"
    "const char *versionNumber = \"SVN revision ${SVN_REVISION}"
)
if(
    SVN_PATH
)
    file(
        APPEND
        "${BINARY_DIR}/version.h.tmp"
        " (${SVN_PATH})"
    )
endif(
    SVN_PATH
)
file(
    APPEND
    "${BINARY_DIR}/version.h.tmp"
    "\";\n"
)
file(
    APPEND
    "${BINARY_DIR}/version.h.tmp"
    "#endif // VERSION_H\n"
)

message(
    STATUS
    "SVN version is "
    ${SVN_REVISION}
)

if(
    EXISTS
    "${BINARY_DIR}/version.h.tmp"
)
    execute_process(
        COMMAND
        ${CMAKE_COMMAND}
        -E
        copy_if_different
        "${BINARY_DIR}/version.h.tmp"
        "${BINARY_DIR}/version.h"
        WORKING_DIRECTORY
        "${SOURCE_DIR}"
    )
    execute_process(
        COMMAND
        ${CMAKE_COMMAND}
        -E
        remove
        "${BINARY_DIR}/version.h.tmp"
        WORKING_DIRECTORY
        "${SOURCE_DIR}"
    )
else(
    EXISTS
    "${BINARY_DIR}/version.h.tmp"
)
    message(
        STATUS
        "${BINARY_DIR}/version.h.tmp does not exist"
    )
endif(
    EXISTS
    "${BINARY_DIR}/version.h.tmp"
)
