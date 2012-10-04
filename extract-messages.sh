#!/bin/sh

dir_resolve()
{
cd "$1" 2>/dev/null || return $?    # cd to desired directory; if fail, quell any error messages but return exit status
echo "`pwd -P`" # output full, link-resolved path
}

# root of translatable sources
BASEDIR=$(dir_resolve "$(dirname $0)")
TRANSLATIONSDIR="${BASEDIR}/translations"

# project name
PROJECT="kbibtex"

BUGADDR="https://gna.org/bugs/?group=kbibtex"
TEMPDIR=$(mktemp -d)

echo "Preparing rc files"

# additional string for KAboutData
echo 'i18nc("NAME OF TRANSLATORS","Your names");' > ${TEMPDIR}/rc.cpp
echo 'i18nc("EMAIL OF TRANSLATORS","Your emails");' >> ${TEMPDIR}/rc.cpp

# we use simple sorting to make sure the lines do not jump around too much from system to system
cd ${BASEDIR}
find ${BASEDIR} -name '*.rc' -o -name '*.ui' -o -name '*.kcfg' | sort | xargs extractrc >>${TEMPDIR}/rc.cpp

echo "Done preparing rc files"


echo "Extracting messages"

# see above on sorting
cd ${BASEDIR}
find ${BASEDIR} -name '*.cpp' -o -name '*.h' -o -name '*.c' | sort >${TEMPDIR}/infiles.list

echo "${TEMPDIR}/rc.cpp" >>${TEMPDIR}/infiles.list

cd ${TEMPDIR}
xgettext --from-code=UTF-8 -C -kde -ci18n -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 -ktr2i18n:1 -kI18N_NOOP:1 -kI18N_NOOP2:1c,2 -kaliasLocale -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3 --msgid-bugs-address="${BUGADDR}" --files-from=${TEMPDIR}/infiles.list -D ${BASEDIR} -D ${TEMPDIR} -o ${TRANSLATIONSDIR}/${PROJECT}.pot || { echo "error while calling xgettext. aborting."; exit 1; }
echo "Done extracting messages"

cd ${BASEDIR}
echo "Merging translations"
for cat in ${TRANSLATIONSDIR}/*.po ; do
  echo $cat
  tempcat=${TEMPDIR}/$(basename "$cat")
  msgmerge -o ${tempcat} $cat ${TRANSLATIONSDIR}/${PROJECT}.pot && mv ${tempcat} $cat
  # remove/rewrite temporary filenames
  sed -i -e 's!'"${TEMPDIR}"'!!g;s!/rc.cpp!rc.cpp!g' $cat
done
echo "Done merging translations"


echo "Cleaning up"
rm -rf ${TEMPDIR}
echo "Done"

