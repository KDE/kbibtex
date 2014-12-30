#!/bin/bash

MY_NAME="$(basename "$0")"
GIT_REPOSITORY=.
RELEASE_NAME="git"
OUTPUT_DIRECTORY="/tmp"
GIT_BRANCH=
GIT_TAG=
GIT_COMMIT=
STEM=
GPG_KEY=
PO_SVN_REVISION=

function get_numeric_release() {
	# The single parameter pass to this function should be something like
	#  0.5
	#  0.5-rc2
	#  0.3-beta1
	local releaseversion=$1

	read numberpart namepart <<<${releaseversion//-/ }

	if [ -z "$namepart" ] ; then
		# There is no "alpha1" or "beta2" provided,
		# so essentially there is nothing to do
		NUMERIC_RELEASE=$numberpart
	else
		# Split number part, e.g. 0.5.1 into individual variables
		read num1 num2 num3 <<<${numberpart//./ }

		if [ -z "${num3}" ] ; then
			# In case there is no third digit ...
			# ... the second number gets decreases by one
			# Example: for 0.5-beta1 we want to have 0.4.xx as numeric version
			NUMERIC_RELEASE="${num1}.$((${num2} - 1))"
		elif [ ${num3} -gt 0 ] ; then
			# In case there is a positive third digit ...
			# ... this third number gets decreases by one
			# Example: for 0.5.4-beta1 we want to have 0.5.3.xx as numeric version
			NUMERIC_RELEASE="${num1}.${num2}.$((${num3} - 1))"
		else
			# Last case: Three digits, assuming second is positive
			# (there is no version x.0.y), and third digit is 0
			# Example: for 0.5.0-beta1 we want to have 0.4.xx as numeric version
			NUMERIC_RELEASE="${num1}.$((${num2} - 1))"
		fi

		if [ $namepart = "test1" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.60"
		elif [ $namepart = "test2" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.61"
		elif [ $namepart = "test3" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.62"
		elif [ $namepart = "test4" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.63"
		elif [ $namepart = "alpha1" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.80"
		elif [ $namepart = "alpha2" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.81"
		elif [ $namepart = "alpha3" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.82"
		elif [ $namepart = "alpha4" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.83"
		elif [ $namepart = "beta1" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.90"
		elif [ $namepart = "beta2" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.91"
		elif [ $namepart = "beta3" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.92"
		elif [ $namepart = "beta4" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.93"
		elif [ $namepart = "rc1" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.95"
		elif [ $namepart = "rc2" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.96"
		elif [ $namepart = "rc3" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.97"
		elif [ $namepart = "rc4" ] ; then NUMERIC_RELEASE="${NUMERIC_RELEASE}.98"
		else NUMERIC_RELEASE="${NUMERIC_RELEASE}.50" ; fi
	fi

	echo "$NUMERIC_RELEASE"
}

function parsearguments {
	local SHOW_HELP=0
	local ARGS=$(getopt -o "h,r:,n:,o:,b:,c:,t:,s:,g:,p:" -l "help,repository:,name:,output-directory:,branch:,commit:,tag:,stem:,gpg-key:,po-svn-revision:" -n $(basename $0) -- "$@")

	#Bad arguments
	if [ $? -ne 0 ] ; then
		exit 1
	fi

	# A little magic
	eval set -- "$ARGS"

	while true ; do
		case "$1" in
		-h|--help)
			SHOW_HELP=1
			shift 1
		;;
		-r|--repository)
			GIT_REPOSITORY="$2"
			shift 2
		;;
		-n|--name)
			RELEASE_NAME="$2"
			shift 2
		;;
		-s|--stem)
			STEM="$2"
			shift 2
		;;
		-o|--output-directory)
			OUTPUT_DIRECTORY="$2"
			shift 2
		;;
		-b|--branch)
			GIT_BRANCH="$2"
			shift 2
		;;
		-c|--commit)
			GIT_COMMIT="$2"
			shift 2
		;;
		-t|--tag)
			GIT_TAG="$2"
			shift 2
		;;
		-g|--gpg-key)
			GPG_KEY="$2"
			shift 2
		;;
		-p|--po-svn-revision)
			PO_SVN_REVISION="$2"
			shift 2
		;;
		--)
			shift
			break
		esac
	done

	if [ ${SHOW_HELP} -ne 0 ] ; then
		echo "Usage: ${MY_NAME} OPTIONS" >&2
		echo "Clone a Git repository, select a branch or tag, clean up things, create a release tar ball from it." >&2
		echo >&2
		echo "Options:" >&2
		echo " --help" >&2
		echo "    Show this help" >&2
		echo " --repository REPOSITORY" >&2
		echo "    Define which repository to clone." >&2
		echo "    Could be a local directory or any git-supported remote location." >&2
		echo "    Required, default: ${GIT_REPOSITORY}">&2
		echo " --branch BRANCH" >&2
		echo "    Git branch to base tar ball on." >&2
		echo "    Either this option or --tag or --commit has to be specified" >&2
		echo "    No default" >&2
		echo " --tag TAG" >&2
		echo "    Git tag to base tar ball on." >&2
		echo "    Either this option or --branch or --commit has to be specified" >&2
		echo "    No default" >&2
		echo " --commit HASH" >&2
		echo "    Git commit to base tar ball on." >&2
		echo "    Either this option or --branch or --tag has to be specified" >&2
		echo "    No default" >&2
		echo "    Usage not recommended, releases should be tagged or based on a branch" >&2
		echo " --name RELEASE_NAME" >&2
		echo "    Release name, should be version number," >&2
		echo "    optionally followed by fancy suffix like \"beta2\"." >&2
		echo "    Optional, default: ${RELEASE_NAME}">&2
		echo " --stem STEM" >&2
		echo "    Stem for archive (both filename and parent directory inside)." >&2
		echo "    Version number (based on --name) will be appended automatically." >&2
		echo "    Required, no default" >&2
		echo " --output-directory DIRECTORY" >&2
		echo "    Where to write the tar ball to." >&2
		echo "    Optional, default: ${OUTPUT_DIRECTORY}">&2
		echo " --gpg-key" >&2
		echo "    GnuPG key used to sign tar ball and cryptographic hash sums." >&2
		echo " --po-svn-revision" >&2
		echo "    WebSVN revision to fetch to get fitting .po translation files" >&2
		echo >&2
		echo "Example:" >&2
		echo "  ${MY_NAME} --branch remotes/origin/kbibtex/0.5 --gpg-key 0xA2641F41 --stem kbibtex --name 0.5-beta3" >&2
		exit 1
	fi

	# Test if provided git repository is local
	if [[ "${GIT_REPOSITORY}" = "${GIT_REPOSITORY/http:/}" && "${GIT_REPOSITORY}" = "${GIT_REPOSITORY/https:/}" && "${GIT_REPOSITORY}" = "${GIT_REPOSITORY/git:/}" && "${GIT_REPOSITORY}" = "${GIT_REPOSITORY/ssh:/}" && "${GIT_REPOSITORY}" = "${GIT_REPOSITORY/rsync:/}" ]] ; then
		# GIT_REPOSITORY must be local directory
		# Test if subdirectory .git exists
		test -d "${GIT_REPOSITORY}/.git" -a -r "${GIT_REPOSITORY}/.git" || { echo "${MY_NAME}: Invalid local Git repository: ${GIT_REPOSITORY} (no .git accessible)" >&2 ; exit 1 ; }
		# Convert to absolute directory:
		GIT_REPOSITORY="$(cd "${GIT_REPOSITORY}" && pwd)"
	fi

	# Test if output directory exists and is writeable
	test -d "${OUTPUT_DIRECTORY}" -a -w "${OUTPUT_DIRECTORY}" || { echo "${MY_NAME}: Invalid output directory: ${OUTPUT_DIRECTORY}" >&2 ; exit 1 ; }
	# Convert to absolute directory:
	OUTPUT_DIRECTORY="$(cd "${OUTPUT_DIRECTORY}" && pwd)"

	# Test if both GIT_BRANCH, GIT_COMMIT, and GIT_TAG are provided
	test -n "${GIT_BRANCH}" -a -n "${GIT_TAG}" -a -n "${GIT_COMMIT}" && { echo "${MY_NAME}: Either a Git branch, tag, or commit hash have to be specified, not all at the same time" >&2 ; exit 1 ; }
	# Test if neither GIT_BRANCH, GIT_COMMIT, nor GIT_TAG are provided
	test -z "${GIT_BRANCH}" -a -z "${GIT_TAG}" -a -z "${GIT_COMMIT}" && { echo "${MY_NAME}: Either a Git branch, tag, or commit hash have to be specified" >&2 ; exit 1 ; }

	# Determine numeric release based on fancy name ("beta2")
	NUMERIC_RELEASE=$(get_numeric_release "${RELEASE_NAME}")

	# Check and sanitize stem
	test -z "${STEM}" && { echo "${MY_NAME}: Stem for the archive name is empty" >&2 ; exit 1 ; }
	STEM="${STEM}-${NUMERIC_RELEASE}"

	# Print used values
	echo "GIT_REPOSITORY=${GIT_REPOSITORY}"
	echo "RELEASE_NAME=${RELEASE_NAME}"
	echo "NUMERIC_RELEASE=${NUMERIC_RELEASE}"
	echo "OUTPUT_DIRECTORY=${OUTPUT_DIRECTORY}"
	echo "STEM=${STEM}"
	echo "GPG_KEY=${GPG_KEY}"
	echo "PO_SVN_REVISION=${PO_SVN_REVISION}"
	test -n "${GIT_BRANCH}" && echo "GIT_BRANCH=${GIT_BRANCH}"
	test -n "${GIT_COMMIT}" && echo "GIT_COMMIT=${GIT_COMMIT}"
	test -n "${GIT_TAG}" && echo "GIT_TAG=${GIT_TAG}"
}


parsearguments "$@"

TEMPDIR=$(mktemp -d || exit 1)

pushd ${TEMPDIR}

# Clone git repository
git clone "${GIT_REPOSITORY}" ${STEM} || { popd ; echo "${MY_NAME}: Could clone from: ${GIT_REPOSITORY}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }

# Go to checkout'ed directory
cd ${STEM}

if [ -n "${GIT_BRANCH}" ] ; then
	# Switch to user-specified branch
	INTERNAL_GIT_BRANCH="${STEM//\//}-tarball"
	git branch "${INTERNAL_GIT_BRANCH}" "${GIT_BRANCH}" || { popd ; echo "${MY_NAME}: Could not branch from: ${GIT_BRANCH}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
	git checkout "${INTERNAL_GIT_BRANCH}"
elif [ -n "${GIT_TAG}" ] ; then
	# Recall user-specified tag
	git checkout "${GIT_TAG}" || { popd ; echo "${MY_NAME}: Could checkout tag: ${GIT_TAG}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
elif [ -n "${GIT_COMMIT}" ] ; then
	# Recall user-specified commit
	INTERNAL_GIT_BRANCH="${STEM//\//}-tarball"
	git branch "${INTERNAL_GIT_BRANCH}" "${GIT_COMMIT}" || { popd ; echo "${MY_NAME}: Could checkout commit: ${GIT_COMMIT}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
	git checkout "${INTERNAL_GIT_BRANCH}"
else
	# This case should never trigger!
	exit 1
fi

# Update release name (e.g. version number) in source files
sed -i -e 's!//const char \*versionNumber!const char \*versionNumber!;s/\(versionNumber\s*=\s*"\).*/\1'${RELEASE_NAME}'";/g;s/#include "version.h"/\nconst char \*versionNumber = "'${RELEASE_NAME}'";/' src/parts/partfactory.cpp src/program/program.cpp || { popd ; echo "${MY_NAME}: Could not change release name to: ${RELEASE_NAME}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
# Update shared library version number
sed -i -e 's/LIB_VERSION \s*"[^"]*"/LIB_VERSION "'${NUMERIC_RELEASE}'"/' CMakeLists.txt || { popd ; echo "${MY_NAME}: Could not change shared library version to: ${RELEASE_NAME}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }

# Remove test code
if [ -d src/test ] ; then
	# Multiline search-and-replace for test directory's "add_subdirectory" statement
	sed -n -i -e '1h;1!H;${;g;s/add_subdirectory\s*(\s*test\s*)//g; p;}' src/CMakeLists.txt || { popd ; echo "${MY_NAME}: Could not remove test code from CMakeLists.txt" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
	rm -rf src/test
fi

# Fetch .po files
mkdir po ; grep -v 'REV=' <download-po-files-from-websvn.sh >download-po-files-from-websvn-without-REV.sh && REV=${PO_SVN_REVISION} bash download-po-files-from-websvn-without-REV.sh
rm -f download-po-files-from-websvn*

# Go to base directory
cd ..

# Create tar ball for full source directory,
# skip various files/directories irrelevant for tar ball,
# and compress tar ball.
TARBALL="${OUTPUT_DIRECTORY}/${STEM}.tar.xz"
tar -c --exclude .svn --exclude .git --exclude testset --exclude '*.sh' --exclude test ${STEM} | nice -n 19 xz >"${TARBALL}"

# Go to output directory to omit absolute filenames in hashsum files
pushd ${OUTPUT_DIRECTORY}
TARBALL_BASENAME="$(basename "${TARBALL}")"
# Create various cryptographic hashes of tar ball
nice md5sum "${TARBALL_BASENAME}" >"${TARBALL}.md5" || { popd ; popd ; echo "${MY_NAME}: Hashing file with md5sum failed: ${TARBALL}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
nice sha1sum "${TARBALL_BASENAME}" >"${TARBALL}.sha1" || { popd ; popd ; echo "${MY_NAME}: Hashing file with sha1sum failed: ${TARBALL}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
nice sha512sum "${TARBALL_BASENAME}" >"${TARBALL}.sha512" || { popd ; popd ; echo "${MY_NAME}: Hashing file with sha512sum failed: ${TARBALL}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
# Leave output directory
popd

# If user specified a GnuPG key to sign tar balls and hashes ...
if [[ -n "${GPG_KEY}" ]] ; then
	# Enumerate all files to sign
	tobesigned="${TARBALL} ${TARBALL}.md5 ${TARBALL}.sha1 ${TARBALL}.sha512"
	for f in ${tobesigned} ; do
		# Force removal of old signatures
		rm -f "${f}.asc"
		# Quick check if file-to-be-signed exists
		test -f "${f}" || { popd ; echo "${MY_NAME}: Could not find file to sign: ${f}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }
		# Sign file using GnuPG
		gpg --output ${f}.asc --default-key ${GPG_KEY} -ba ${f} || { popd ; echo "${MY_NAME}: Could not sign \"${f}\" using GPG key ${GPG_KEY}" >&2 ; rm -rf ${TEMPDIR} ; exit 1 ; }

		# Print file and signature file
		ls -l "${f}" "${f}.asc"
		# Check signature
		gpg --verify "${f}.asc"
	done
fi

popd

# Remove all temporary files
rm -rf ${TEMPDIR}
