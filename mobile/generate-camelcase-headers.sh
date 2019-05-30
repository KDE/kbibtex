#!/usr/bin/env bash

camelcaseheadernames=(BibTeXEntries BibTeXFields Comment Element Encoder EncoderXML Entry File FileImporter FileImporterBibTeX KBibTeX Macro OnlineSearchAbstract Preamble Preferences Value XSLTransform)

if [[ $# != 2 || $1 == "-h" || $1 == "--help" ]] ; then
	echo "Generate CamelCase headers in a target directory based on"
	echo "lower-case headers in a source directory."
	echo
	echo "Two parameters are expected:"
	echo " 1. The absolute path to KBibTeX's 'src/' directory."
	echo " 2. The absolute path to the the 'src/' directory inside"
	echo "    the build directory. If the directory does not yet"
	echo "    exist, it will be created."
	echo
	echo "Example:"
	echo "  ./generate-camelcase-headers.sh ~/git/kbibtex/src/ ~/git/build-bibsearch-SailfishOS_3_0_3_9_i486_in_Sailfish_OS_Build_Engine-Debug/src/"
	echo
	echo "The following CamelCase headers will be generated:"
	for camelcaseheadername in "${camelcaseheadernames[@]}" ; do echo "  ${camelcaseheadername}" ; done
	exit 1
fi

sourcedir="$1"
destdir="$2"
test -d "${sourcedir}" || { echo "Not a directory: '${sourcedir}'" >&2 ; exit 1 ; }
test -d "${destdir}" || { echo "Creating directory: '${destdir}'" >&2 ; mkdir -p "${destdir}" ; }

for camelcaseheadername in "${camelcaseheadernames[@]}" ; do
	lowercaseheadername="$(tr 'A-Z' 'a-z' <<<"${camelcaseheadername}").h"
	absolutelowercasefilename="$(find "${sourcedir}" -name "${lowercaseheadername}" | head -n 1)"
	test -s "${absolutelowercasefilename}" || { echo "No lowercase header name found for '${camelcaseheadername}': '${lowercaseheadername}'" >&2 ; exit 1 ; }
	absolutecamelcasefilename="$(dirname "${absolutelowercasefilename/$sourcedir/$destdir}")/${camelcaseheadername}"
	mkdir -p "$(dirname "${absolutecamelcasefilename}")" || exit 1
	echo "#include \"${lowercaseheadername}\"" >"${absolutecamelcasefilename}"
done
