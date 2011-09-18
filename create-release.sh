#!/bin/sh

svnsource=$1
releaseversion=$2
outputdir=$3
tempdir="$(mktemp --tmpdir=/tmp/ kbibtex-create-release-XXXXXX || exit 3)"
rm -rf ${tempdir} ; mkdir -p ${tempdir} || exit 2

if [ -z "${svnsource}" ] ; then
	svnsource="trunk"
	echo "No SVN source as first parameter specified"
	echo " Example \"tags/0.4\""
	echo " Falling back to \"$svnsource\""
	echo
fi

if [ -z "${releaseversion}" ] ; then
	releaseversion="svn"
	echo "No release version as second parameter specified"
	echo " Example \"0.4\""
	echo " Falling back to \"$releaseversion\""
	echo
fi

if [ -z "${outputdir}" ] ; then
	outputdir="${PWD}"
	echo "No output directory as third parameter specified"
	echo " Example \"/tmp/\""
	echo " Falling back to \"$outputdir\""
	echo
fi

archivename="kbibtex-${releaseversion}.tar.bz2"

read -p "Continue to create tar ball \"${archivename}\"? [yN] " answer || exit 10
test "$answer" = "y" -o "$answer" = "Y" || exit 11

echo "Preparing to create archive $archivename ..."

echo -n "Switching to temporary directory "
pushd "${tempdir}" >/dev/null || exit 2

echo "Fetching sources from SVN: svn://svn.gna.org/svn/kbibtex/${svnsource}"
svn co -q svn://svn.gna.org/svn/kbibtex/${svnsource} kbibtex-${releaseversion} || exit 4

if [ ${releaseversion} != "svn" ] ; then
	echo "Changing version number in source to ${releaseversion}"
	sed -i -e 's/\bversionNumber\b/"'${releaseversion}'"/g' kbibtex-${releaseversion}/src/parts/partfactory.cpp kbibtex-${releaseversion}/src/program/program.cpp || exit 5
fi

echo "Compressing source code into archive"
tar -jcf "${outputdir}/${archivename}" --exclude .svn kbibtex-${releaseversion} || exit 6

popd
echo "Cleaning up temporary directory"
rm -rf "${tempdir}"

echo "Creating checksums"
nice md5sum "${outputdir}/${archivename}" >"${outputdir}/${archivename}.md5" || exit 7
nice sha1sum "${outputdir}/${archivename}" >"${outputdir}/${archivename}.sha1" || exit 7
nice sha512sum "${outputdir}/${archivename}" >"${outputdir}/${archivename}.sha512" || exit 7

echo "Signing archive and checksums"
tobesigned="${outputdir}/${archivename} ${outputdir}/${archivename}.md5 ${outputdir}/${archivename}.sha1 ${outputdir}/${archivename}.sha512"
for f in ${tobesigned} ; do rm -f "${f}.asc" ; gpg --output ${f}.asc --default-key 0xA264FD738D861F41 -ba ${f} || exit 8 ; done

echo "Done"
echo
ls -l "${outputdir}/${archivename}"*

