#!/usr/bin/env bash

KDELIBSTAR=$(ls -1 /usr/portage/distfiles/kdelibs-4.*.tar.* | tail -n 1)
TEMPDIR=$(mktemp -d)
APIDOXDIR="/tmp/kbibtex-apidox"

tar -xvf ${KDELIBSTAR} --wildcards -C ${TEMPDIR} 'kdelibs-4.*/doc/api' 'kdelibs-4.*/doc/common'
KDELIBSTEMPDIR=$(ls -1d ${TEMPDIR}/kdelibs-4.*)

mkdir -p ${APIDOXDIR}
rm -rf "${APIDOXDIR}/*"

pushd ${APIDOXDIR}
${KDELIBSTEMPDIR}/doc/api/doxygen.sh --title="KBibTeX" --recurse --doxdatadir=${KDELIBSTEMPDIR}/doc/common ~/Programming/GNA/kbibtex/
popd

rm -rf ${TEMPDIR}
