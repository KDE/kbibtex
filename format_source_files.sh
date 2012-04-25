#!/bin/sh

export LC_ALL=en_US.utf8
export LANG=C

astyle --align-reference=name --align-pointer=name --indent=spaces=4 --brackets=linux --indent-labels --pad-oper --unpad-paren --pad-header --keep-one-line-statements --convert-tabs --indent-preprocessor $(find src -type f -name '*.cpp' -o -name '*.h')

# correct some astyle mis-formattings
find src -name '*.h' -exec sed -i -e  's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g' '{}' ';'
find src -name '*.cpp' -exec sed -i -e  's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g;s/\(foreach([^)]*\) \([&*]\) /\1 \2/g' '{}' ';'

export CMAKEPP=$(which cmakepp)
test -x "${CMAKEPP}" && find -name CMakeLists.txt -exec ${CMAKEPP} --sob 0 --overwrite '{}' ';'
