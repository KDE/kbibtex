#!/usr/bin/env bash

export LC_ALL=en_US.utf8
export LANG=C
MY_NAME="$(basename "$0")"

CURYEAR="$(date '+%Y')"
ALL_FILES=0
HEAD_COMMIT=0

function parsearguments {
	local ARGS PRINT_HELP
	ARGS=$(getopt -o "h" -l "verbose,all,head,help" -n "${MY_NAME}" -- "$@")
	GETOPT_EXIT=$?
	PRINT_HELP=0

	#Bad arguments
	(( GETOPT_EXIT == 0 )) || exit 1

	# A little magic
	eval set -- "$ARGS"

	while true ; do
		case "$1" in
		-h|--help)
			PRINT_HELP=1
			shift
		;;
		--all )
			ALL_FILES=1
			HEAD_COMMIT=0
			shift
		;;
		--head )
			HEAD_COMMIT=1
			ALL_FILES=0
			shift
		;;
		--)
			shift
			break
		esac
	done

	if [[ ${PRINT_HELP} -ne 0 ]] ; then
		echo "Usage: ${MY_NAME} [--all|--head] [--verbose]" >&2
		echo "Format C++ source code and headers in a git-controlled directory structure." >&2
		echo >&2
		echo "Options:" >&2
		echo " --help" >&2
		echo "    Show this help" >&2
		echo " --verbose" >&2
		echo "    Show verbose (more) output" >&2
		echo " --head" >&2
		echo "    Format .cpp/.h files changed in current HEAD commit">&2
		echo " --all" >&2
		echo "    Format all .cpp/.h files, not just those marked by git as added or modified">&2
		exit 1
	fi
}

parsearguments "$@"

if [[ ${HEAD_COMMIT} -eq 1 ]] ; then
	git diff-tree --no-commit-id --name-only -r HEAD | grep -E '[.](cpp|h)$'
elif [[ ${ALL_FILES} -eq 0 ]] ; then
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
grep --color=NEVER -vE '(src/config/preferences.|src/networking/onlinesearch/onlinesearch[^.]+-parser.in.)(cpp|h)$' | \
while read -r filename ; do
	if [[ ! -s "${filename}" ]] ; then
		echo "${MY_NAME}: File not found: \"${filename}\"" >&2
		continue
	fi

	TEMPFILE=$(mktemp -t 'kbibtex_formatted_source_XXXXXXXXXXX.cpp')

	echo "Processing \"${filename}\""
	astyle -n --align-reference=name --align-pointer=name --indent=spaces=4 --indent-labels --pad-oper --unpad-paren --pad-header --keep-one-line-statements --convert-tabs <"${filename}" >"${TEMPFILE}" || exit 1

	# Astyle has problem with 'foreach' statements
	sed -i -e 's/\bforeach(/foreach (/g' "${TEMPFILE}" || exit 1

	# normalize SIGNAL statements as recommended at
	# http://marcmutz.wordpress.com/effective-qt/prefer-to-use-normalised-signalslot-signatures/
	# astyle would insert spaces etc
	while read -r original ; do
		normalized="$(sed -e 's/const //g;s/&//g;s/ //g;s/\//\\\//g' <<<"${original}")"
		originalregexpized="$(sed -e 's/\*/\\*/g;s/\//\\\//g' <<<"${original}")"
		sed -i -e 's/'"${originalregexpized}"'/'"${normalized}"'/g' "${TEMPFILE}"
	done < <(grep -Po '(SIGNAL|SLOT)\([^(]+\([^)]*\)\)' "${TEMPFILE}") || exit 1

	# correct some astyle mis-formattings
	if [[ "${filename}" != "${filename/%.h/}" ]] ; then
		sed -i -e 's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g' "${TEMPFILE}"
	elif [[ "${filename}" != "${filename/%.cpp/}" ]] ; then
		sed -i -e  's/^\([ ]*\)\(:[ ][a-zA-Z]\)/\1    \2/g;s/\(foreach([^)]*\) \([&*]\) /\1 \2/g' "${TEMPFILE}"
	fi || exit 1

	# In const_cast operations, ensure space before ampersand
	sed -i -r 's!( : const_cast<[^(;]+[^ ])&>[(]!\1 \&>(!g' "${TEMPFILE}" || exit 1

	# Remove superfluous empty lines at file's end
	#  http://unix.stackexchange.com/a/81687
	#  http://sed.sourceforge.net/sed1line.txt
	sed -i -e :a -e '/^\n*$/{$d;N;};/\n$/ba' "${TEMPFILE}" || exit 1

	# Check if copyright year is up to date
	while read -r year rest ; do
		[ "${year}" = "${CURYEAR}" ] || echo "File '${filename}' has an outdated copyright year: ${year}"
	done < <(head -n 5 "${TEMPFILE}" | grep -Eo --color=NEVER '20[0-9][0-9] Thomas' | head -n 1)

	# only change/touch original file if it has been changed
	diff -q "${TEMPFILE}" "${filename}" >/dev/null || { echo "Updating \"${filename}\"" ; cp -p "${TEMPFILE}" "${filename}" || echo "Cannot copy \"${TEMPFILE}\" \"${filename}\"" ; }

	rm -f "${TEMPFILE}" || exit 1
done

test -s README && { grep -q "2004-${CURYEAR} Thomas Fischer" README || echo "README should have current year in copyright: ${CURYEAR}" ; }
for copyrightfile in src/program/program.cpp src/test/main.cpp src/parts/partfactory.cpp ; do
	test -s "${copyrightfile}" && { grep -q "SPDX-FileCopyrightText: 2004-${CURYEAR} Thomas Fischer" "${copyrightfile}" || echo "'${copyrightfile}' should have current year in copyright: ${CURYEAR}" ; }
done
