#!/bin/sh

# Usage examples
#  ./create-release.sh tags/0.4 0.4 /tmp/
#  ./create-release.sh trunk head .
#  ./create-release.sh branches/0.4 0.4.9999 ~/public_html/

svnsource=$1
releaseversion=$2
outputdir=$3
tempdir="$(mktemp --tmpdir=/tmp/ kbibtex-create-release-XXXXXX || exit 3)"
rm -rf ${tempdir} ; mkdir -p ${tempdir} || exit 2

function getnumericreleaseversion() {
	local releaseversion=$1

	read numberpart namepart <<<${releaseversion//-/ }

	if [ -z "$namepart" ] ; then
		numericreleaseversion=$numberpart
	else
		read num1 num2 num3 <<<${numberpart//./ }

		if [ -z "${num3}" ] ; then
			numericreleaseversion="${num1}.$((${num2} - 1))"
		elif [ ${num3} -gt 0 ] ; then
			numericreleaseversion="${num1}.${num2}.$((${num3} - 1))"
		else
			numericreleaseversion="${num1}.$((${num2} - 1))"
		fi

		if [ $namepart = "alpha1" ] ; then numericreleaseversion="${numericreleaseversion}.80"
		elif [ $namepart = "alpha2" ] ; then numericreleaseversion="${numericreleaseversion}.81"
		elif [ $namepart = "alpha3" ] ; then numericreleaseversion="${numericreleaseversion}.82"
		elif [ $namepart = "alpha4" ] ; then numericreleaseversion="${numericreleaseversion}.83"
		elif [ $namepart = "beta1" ] ; then numericreleaseversion="${numericreleaseversion}.90"
		elif [ $namepart = "beta2" ] ; then numericreleaseversion="${numericreleaseversion}.91"
		elif [ $namepart = "beta3" ] ; then numericreleaseversion="${numericreleaseversion}.92"
		elif [ $namepart = "beta4" ] ; then numericreleaseversion="${numericreleaseversion}.93"
		elif [ $namepart = "rc1" ] ; then numericreleaseversion="${numericreleaseversion}.95"
		elif [ $namepart = "rc2" ] ; then numericreleaseversion="${numericreleaseversion}.96"
		elif [ $namepart = "rc3" ] ; then numericreleaseversion="${numericreleaseversion}.97"
		elif [ $namepart = "rc4" ] ; then numericreleaseversion="${numericreleaseversion}.98"
		else numericreleaseversion="${numericreleaseversion}.50" ; fi
	fi

	echo "$numericreleaseversion"
}

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

numericreleaseversion=$(getnumericreleaseversion $releaseversion)
archivename="kbibtex-${releaseversion}.tar.bz2"

read -p "Continue to create tar ball \"${outputdir}/${archivename}\"? [yN] " answer || exit 10
test "$answer" = "y" -o "$answer" = "Y" || exit 11

echo "Preparing to create archive $archivename ..."

echo -n "Switching to temporary directory "
pushd "${tempdir}" >/dev/null || exit 2

echo "Fetching sources from SVN: svn://svn.gna.org/svn/kbibtex/${svnsource}"
svn co -q svn://svn.gna.org/svn/kbibtex/${svnsource} kbibtex-${releaseversion} || exit 4

# Dump SVN information into a file
svn info kbibtex-${releaseversion} | grep -v -E '^Revision: ' >kbibtex-${releaseversion}/svn-info.txt
svn info kbibtex-${releaseversion} | awk '/^Last Changed Rev:/ {print $NF}' >kbibtex-${releaseversion}/svn-revision.txt

if [ ${releaseversion} != "svn" ] ; then
	echo "Changing version number in source to ${releaseversion}"
	sed -i -e 's/\bversionNumber\b/"'${releaseversion}'"/g' kbibtex-${releaseversion}/src/parts/partfactory.cpp kbibtex-${releaseversion}/src/program/program.cpp || exit 5

	sed -i -e 's/LIB_VERSION "[^"]*"/LIB_VERSION "'${numericreleaseversion}'"/' kbibtex-${releaseversion}/CMakeLists.txt
fi

# Remove test code
if [[ -d kbibtex-${releaseversion}/src/test ]] ; then
	# multiline search-and-replace for test directory's "add_subdirectory" statement
	sed -i -e '1h;1!H;${;g;s/add_subdirectory\s*(\s*test\s*)//;p;}' kbibtex-${releaseversion}/src/CMakeLists.txt
	rm -rf kbibtex-${releaseversion}/src/test
fi

echo "Compressing source code into archive"
mkdir -p "${outputdir}"
tar -jcf "${outputdir}/${archivename}" --exclude .svn --exclude testset kbibtex-${releaseversion} || exit 6

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

