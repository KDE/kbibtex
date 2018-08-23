#!/bin/sh

export LC_ALL=en_US.utf8
export LANG=C
MY_NAME="$(basename "$0")"

SET_VERBOSE=0
ALL_FILES=0

function parsearguments {
	local ARGS=$(getopt -o "h" -l "verbose,all,help" -n "${MY_NAME}" -- "$@")
	local PRINT_HELP=0

	#Bad arguments
	if [ $? -ne 0 ] ; then
		exit 1
	fi

	# A little magic
	eval set -- "$ARGS"

	while true ; do
		case "$1" in
		-h|--help)
			PRINT_HELP=1
			shift
		;;
		--verbose )
			SET_VERBOSE=1
			shift
		;;
		--all )
			ALL_FILES=1
			shift
		;;
		--)
			shift
			break
		esac
	done

	if [[ ${PRINT_HELP} -ne 0 ]] ; then
		echo "Usage: ${MY_NAME} [--all] [--verbose]" >&2
		echo "Format C++ source code and headers in a git-controlled directory structure." >&2
		echo >&2
		echo "Options:" >&2
		echo " --help" >&2
		echo "    Show this help" >&2
		echo " --verbose" >&2
		echo "    Show verbose (more) output" >&2
		echo " --all" >&2
		echo "    Format all .cpp/.h files, not just those marked by git as added or modified">&2
		exit 1
	fi
}

parsearguments "$@"

if [[ ${ALL_FILES} -eq 0 ]] ; then
	# Format only files seen by git as added or modified,
	# filter for .cpp or .h files.
	# Trying to handle filenames with spaces correctly (not tested)
	git status --untracked-files=no --porcelain | awk '/^\s*[AM]+\s+.*[.](cpp|h)$/ {for(i=2;i<NF;++i){printf $i" "};printf $NF"\n"}'
else
	# Format all .cpp or .h files; using "find" to located those files
	find . -type f -name '*.cpp' -o -name '*.h'
fi | \
# Sort files alphabetically, omit duplicates
sort -u | \
while read filename ; do
	if [[ ! -s "${filename}" ]] ; then
		echo "${MY_NAME}: File not found: \"${filename}\"" >&2
		continue
	fi

	TEMPFILE=$(mktemp -t 'kbibtex_formatted_source_XXXXXXXXXXX.cpp')

	echo "Processing \"${filename}\""
	astyle -n --align-reference=name --align-pointer=name --indent=spaces=4 --indent-labels --pad-oper --unpad-paren --pad-header --keep-one-line-statements --convert-tabs --indent-preprocessor <"${filename}" >${TEMPFILE}

	# Astyle has problem with 'foreach' statements
	sed -i -e 's/\bforeach(/foreach (/g' ${TEMPFILE}

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

	# In const_cast operations, ensure space before ampersand
	sed -i -r 's!( : const_cast<[^(;]+[^ ])&>[(]!\1 \&>(!g' ${TEMPFILE}

	# Remove superfluous empty lines at file's end
	#  http://unix.stackexchange.com/a/81687
        #  http://sed.sourceforge.net/sed1line.txt
	sed -i -e :a -e '/^\n*$/{$d;N;};/\n$/ba' ${TEMPFILE}

	# Check if copyright year is up to date
	head -n 5 ${TEMPFILE} | grep -Eo --color=NEVER '20[0-9][0-9] by ' | head -n 1 | while read year rest ; do
		[ "${year}" = $(date '+%Y') ] || echo "File '${filename}' has an outdated copyright year: ${year}"
	done

	# only change/touch original file if it has been changed
	diff -q ${TEMPFILE} "${filename}" >/dev/null || { echo "Updating \"${filename}\"" ; cp -p ${TEMPFILE} "${filename}" || echo "Cannot copy \"${TEMPFILE}\" \"${filename}\"" ; }

	rm -f ${TEMPFILE}
done

year=$(date '+%Y')
test -s README && { grep -q "2004-${year} Thomas Fischer" README || echo "README should have current year in copyright: ${year}" ; }
for copyrightfile in src/program/program.cpp src/test/main.cpp src/parts/partfactory.cpp ; do
    test -s "${copyrightfile}" && { grep -q "\"Copyright 2004-${year} Thomas Fischer" "${copyrightfile}" || echo "'${copyrightfile}' should have current year in copyright: ${year}" ; }
done
