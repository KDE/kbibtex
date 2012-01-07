#!/bin/sh

export LC_ALL=en_US.utf8
export LANG=C

astyle --indent=spaces=4 --brackets=linux --indent-labels --pad=oper --unpad=paren --one-line=keep-statements --convert-tabs --indent-preprocessor $(find src -type f -name '*.cpp' -o -name '*.h')

export CMAKEPP=$(which cmakepp)
test -x "${CMAKEPP}" && find -name CMakeLists.txt -exec ${CMAKEPP} --sob 0 --overwrite '{}' ';'
