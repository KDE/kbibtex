#!/usr/bin/env bash

export LANG=en_US.utf8

test -d po || { echo "Need to have a po/ directory to run safely!" >&2 ; exit 1 ; }

# REV= # KBibTeX master
REV=1409562 # KBibTeX 0.6.0

if [ -n "${REV}" ] ; then
	echo "Using websvn revision ${REV}"
	# Rewriting part of the URL that contains the revision number
	REV="?revision=${REV}"
fi

# For all languages KDE4 supports ...
for lang in af ar as ast be be@latin bg bn bn_IN br bs ca ca@valencia crh cs csb cy da de el en_GB eo es et eu fa fi fr fy ga gl gu ha he hi hne hr hsb hu hy ia id is it ja ka kk km kn ko ku lb lt lv mai mk ml mr ms mt nb nds ne nl nn nso oc or pa pl ps pt pt_BR ro ru rw se si sk sl sq sr sr@ijekavian sr@ijekavianlatin sr@latin sv ta te tg th tr tt ug uk uz uz@cyrillic vi wa xh zh_CN zh_HK zh_TW ; do
	# Create temporary directory
	TEMPDIR=$(mktemp -d)
	# Download all .po files for this language for KBibTeX
	# using the requested revision into the language's subdirectory
	curl --silent "http://websvn.kde.org/*checkout*/trunk/l10n-kde4/${lang}/messages/extragear-office/{kbibtex.po,kbibtex_xml_mimetypes.po,desktop_extragear-office_kbibtex.po,kbibtex.appdata.po}${REV}" -o "${TEMPDIR}/#1" || {
		# If curl failed, try to remove language's subdirectory
		rm -rf "${TEMPDIR}" "po/${lang}" ; echo "Language '${lang}' not supported" >&2 ; continue ;
	}
	test -s ${TEMPDIR}/kbibtex.po || {
		# No .po files downloaded
		rm -rf "${TEMPDIR}" "po/${lang}" ; echo "Language '${lang}' not supported" >&2 ; continue ;
	}
	test -s ${TEMPDIR}/kbibtex.appdata.po || {
		# No .po files downloaded
		rm -rf "${TEMPDIR}" "po/${lang}" ; echo "Language '${lang}' not supported" >&2 ; continue ;
	}
	test -s ${TEMPDIR}/desktop_extragear-office_kbibtex.po || {
		# No .po files downloaded
		rm -rf "${TEMPDIR}" "po/${lang}" ; echo "Language '${lang}' not supported" >&2 ; continue ;
	}
	grep -q "ViewVC" ${TEMPDIR}/kbibtex.po && {
		# If .po files contain error message instead of translation,
		# try to erase language's subdirectory including its content
		rm -rf "${TEMPDIR}" "po/${lang}" ; echo "Language '${lang}' not supported" >&2 ; continue ;
	}
	grep -q "ViewVC" ${TEMPDIR}/kbibtex.appdata.po && {
		# If .po files contain error message instead of translation,
		# try to erase language's subdirectory including its content
		rm -rf "${TEMPDIR}" "po/${lang}" ; echo "Language '${lang}' not supported" >&2 ; continue ;
	}
	grep -q "ViewVC" ${TEMPDIR}/desktop_extragear-office_kbibtex.po && {
		# If .po files contain error message instead of translation,
		# try to erase language's subdirectory including its content
		rm -rf "${TEMPDIR}" "po/${lang}" ; echo "Language '${lang}' not supported" >&2 ; continue ;
	}

	# Create subdirectory for language's .po files
	# by moving temporary directory in its place
	rm -rf "po/${lang}" ; mv "${TEMPDIR}" "po/${lang}"
	echo "Language '${lang}' successfully downloaded"
done

cat <<EOF >po/CMakeLists.txt
find_package(Gettext REQUIRED)

if (NOT GETTEXT_MSGMERGE_EXECUTABLE)
    message(FATAL_ERROR "Please install the msgmerge program from the gettext package.")
endif (NOT GETTEXT_MSGMERGE_EXECUTABLE)

if (NOT GETTEXT_MSGFMT_EXECUTABLE)
    message(FATAL_ERROR "Please install the msgfmt program from the gettext package.")
endif (NOT GETTEXT_MSGFMT_EXECUTABLE)

EOF

cd po
# For each language subdirectory ...
ls -1d [a-z][a-z] [a-z][a-z][@_][a-z][a-z]* | while read lang ; do
	# Add a new line to CMakeLists.txt
	echo "add_subdirectory(${lang})"

	# Create language-specific LANGUAGE/CMakeLists.txt file
	# (overwriting existing one)
	echo 'file(GLOB _po_files *.po)' >${lang}/CMakeLists.txt
	echo 'gettext_process_po_files('${lang}' ALL INSTALL_DESTINATION ${LOCALE_INSTALL_DIR} ${_po_files} )' >>${lang}/CMakeLists.txt
done >>CMakeLists.txt
cd ..
