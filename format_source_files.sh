#!/bin/sh

export LC_ALL=en_US.utf8
export LANG=C

find src -type f -name '*.cpp' -o -name '*.h' | xargs git status | while read hash status filename ; do
	if [[ ${hash} != "#" ]] ; then continue ; fi
	if [[ ${status} != "modified:" && ${status} != "added:" ]] ; then continue ; fi
	if [[ ! -s "${filename}" ]] ; then continue ; fi

	echo "Processing \"${filename}\""
	astyle --align-reference=name --align-pointer=name --indent=spaces=4 --brackets=linux --indent-labels --pad-oper --unpad-paren --pad-header --keep-one-line-statements --convert-tabs --indent-preprocessor "${filename}"

	# normalize SIGNAL statements as recommended at
	# http://marcmutz.wordpress.com/effective-qt/prefer-to-use-normalised-signalslot-signatures/
	# astyle would insert spaces etc
	grep -Po 'SIGNAL\([^(]+\([^)]*\)\)' "${1}" | while read original ; do
		normalized="$(sed -e 's/const //g;s/&//g;s/ //g;s/\//\\\//g' <<<${original})"
		originalregexpized="$(sed -e 's/\*/\\*/g;s/\//\\\//g' <<<${original})"
		sed -i -e 's/'"${originalregexpized}"'/'"${normalized}"'/g' "${1}"
	done

	# correct some astyle mis-formattings
	if [[ "${filename}" != "${filename/%.h/}" ]] ; then
		sed -i -e  's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g' "${filename}"
	elif [[ "${filename}" != "${filename/%.cpp/}" ]] ; then
		sed -i -e  's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g;s/\(foreach([^)]*\) \([&*]\) /\1 \2/g' "${filename}"
	fi
done

