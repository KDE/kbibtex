#############################################################################
#   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   #
#                                                                           #
#   This program is free software; you can redistribute it and/or modify    #
#   it under the terms of the GNU General Public License as published by    #
#   the Free Software Foundation; either version 2 of the License, or       #
#   (at your option) any later version.                                     #
#                                                                           #
#   This program is distributed in the hope that it will be useful,         #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of          #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
#   GNU General Public License for more details.                            #
#                                                                           #
#   You should have received a copy of the GNU General Public License       #
#   along with this program; if not, see <http://www.gnu.org/licenses/>.    #
#############################################################################

# Inspired by:
# http://stackoverflow.com/questions/3780667/use-cmake-to-get-build-time-svn-revision

if (DEFINED ENV{GIT_INFO})
    message (STATUS "Git information set by environment variable GIT_INFO")
    set (GIT_INFO $ENV{GIT_INFO})
else (DEFINED ENV{GIT_INFO})
    set(GIT_INFO "unknown")

    # Git
    find_program( GIT_EXECUTABLE NAMES git.bat git ) # for Windows, "git.bat" must be found before "git"
    if (GIT_EXECUTABLE)
        if (EXISTS ${SOURCE_DIR}/.git)
            execute_process (
                WORKING_DIRECTORY
                "${SOURCE_DIR}"
                COMMAND
                ${GIT_EXECUTABLE} rev-parse --short HEAD
                OUTPUT_VARIABLE GIT_REV
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            execute_process (
                WORKING_DIRECTORY
                "${SOURCE_DIR}"
                COMMAND
                ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            set(GIT_INFO "${GIT_REV} (${GIT_BRANCH})" )
        else (EXISTS ${SOURCE_DIR}/.git)
            message( "Not a Git source directory" )
        endif (EXISTS ${SOURCE_DIR}/.git)
    else (GIT_EXECUTABLE)
        message( "No Git executable" )
    endif (GIT_EXECUTABLE)
endif (DEFINED ENV{GIT_INFO})

# write a header file defining VERSION
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
    "const char *versionNumber = \"Git revision ${GIT_INFO}"
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
    "Git revision is "
    ${GIT_INFO}
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
