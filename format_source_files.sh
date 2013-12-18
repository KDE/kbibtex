#!/bin/sh

export LC_ALL=en_US.utf8
export LANG=C
MY_NAME="$(basename "$0")"

find src -type f -name '*.cpp' -o -name '*.h' | xargs git status --untracked-files=no --porcelain | while read status filename ; do
	if [[ ${status} != "M" && ${status} != "A" ]] ; then continue ; fi
	if [[ ! -s "${filename}" ]] ; then
		echo "${MY_NAME}: File not found: \"${filename}\" (state: ${status})" >&2
		continue
	fi

	TEMPFILE=$(mktemp 'kbibtex_formatted_source_XXXXXXXXXXX.cpp')

	echo "Processing \"${filename}\" (state: ${status})"
	astyle -n --align-reference=name --align-pointer=name --indent=spaces=4 --brackets=linux --indent-labels --pad-oper --unpad-paren --pad-header --keep-one-line-statements --convert-tabs --indent-preprocessor <"${filename}" >${TEMPFILE}

	# normalize SIGNAL statements as recommended at
	# http://marcmutz.wordpress.com/effective-qt/prefer-to-use-normalised-signalslot-signatures/
	# astyle would insert spaces etc
	grep -Po '(SIGNAL|SLOT)\([^(]+\([^)]*\)\)' "${TEMPFILE}" | while read original ; do
		normalized="$(sed -e 's/const //g;s/&//g;s/ //g;s/\//\\\//g' <<<${original})"
		originalregexpized="$(sed -e 's/\*/\\*/g;s/\//\\\//g' <<<${original})"
		sed -i -e 's/'"${originalregexpized}"'/'"${normalized}"'/g' ${TEMPFILE}
	done

	# correct some astyle mis-formattings
	if [[ "${filename}" != "${filename/%.h/}" ]] ; then
		sed -i -e 's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g' ${TEMPFILE}
	elif [[ "${filename}" != "${filename/%.cpp/}" ]] ; then
		sed -i -e  's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g;s/\(foreach([^)]*\) \([&*]\) /\1 \2/g' ${TEMPFILE}
	fi

	# only change/touch original file if it has been changed
	diff -q ${TEMPFILE} "${filename}" >/dev/null || cp -p ${TEMPFILE} "${filename}"

	rm -f ${TEMPFILE}
done

