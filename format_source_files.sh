#!/bin/sh

export LC_ALL=en_US.utf8
export LANG=C

find src -type f -name '*.cpp' -o -name '*.h' | xargs git status | while read hash status filename ; do
	if [[ ${hash} != "#" ]] ; then continue ; fi
	if [[ ${status} != "modified:" && ${status} != "added:" ]] ; then continue ; fi
	if [[ ! -s "${filename}" ]] ; then continue ; fi

	TEMPFILE=$(mktemp 'kbibtex_formatted_source_XXXXXXXXXXX.cpp')

	echo "Processing \"${filename}\""
	astyle -n --align-reference=name --align-pointer=name --indent=spaces=4 --brackets=linux --indent-labels --pad-oper --unpad-paren --pad-header --keep-one-line-statements --convert-tabs --indent-preprocessor <"${filename}" >${TEMPFILE}

	# correct some astyle mis-formattings
	if [[ "${filename}" != "${filename/%.h/}" ]] ; then
		sed -i -e 's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g' ${TEMPFILE}
	elif [[ "${filename}" != "${filename/%.cpp/}" ]] ; then
		sed -i -e 's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g;s/\(foreach([^)]*\) \([&*]\) /\1 \2/g' ${TEMPFILE}
	fi

	# only change/touch original file if it has been changed
	diff -q ${TEMPFILE} "${filename}" >/dev/null || cp -p ${TEMPFILE} "${filename}"

	rm -f ${TEMPFILE}
done

