#!/usr/bin/env bash

set -euo pipefail

finalimagename=""

TEMPDIR="$(mktemp --tmpdir -d "$(basename "${0/.sh}")"-XXXXX.d)"
export TEMPDIR
function cleanup_on_exit {
	rm -rf "${TEMPDIR}"
}
trap cleanup_on_exit EXIT

function buildahsetx() {
	set -x
	buildah "${@}"
	exitcode=$?
	{ set +x ; } 2>/dev/null
	return ${exitcode}
}

function podmansetx() {
	set -x
	podman "${@}"
	exitcode=$?
	{ set +x ; } 2>/dev/null
	return ${exitcode}
}

function create_directories() {
	local id
	id=$1

	for d in source build xdg-config-home kbibtex-podman ; do
		buildahsetx run --user root "${id}" -- mkdir -p "/tmp/${d}" || exit 1
	done
	for d in runtime cache config ; do
		buildahsetx run --user root "${id}" -- mkdir -m 700 -p "/tmp/xdg-${d}-dir" || exit 1
	done
}

function set_environment() {
	buildahsetx config --env QT_PLUGIN_PATH=/usr/lib/qt/plugins --env QT_X11_NO_MITSHM=1 --env KDEDIRS=/usr --env XDG_RUNTIME_DIR=/tmp/xdg-runtime-dir --env XDG_CACHE_HOME=/tmp/xdg-cache-home --env XDG_CONFIG_HOME=/tmp/xdg-config-home --env XDG_DATA_DIRS=/usr/share --env XDG_DATA_HOME=/tmp/xdg-data-home --env XDG_CURRENT_DESKTOP=KDE "$1"
}

function create_user() {
	local id
	id=$1
	local username
	username="${2:-kdeuser}"

	echo "Creating user '${username}' in working container '${id}'"

	[[ "${username}" != "neon" ]] && buildahsetx run --user root "${id}" -- useradd --home-dir /tmp --no-log-init --no-create-home --shell /bin/bash --user-group "${username}"
	for d in source build xdg-config-home kbibtex-podman ; do
		buildahsetx run --user root "${id}" -- chown -R "${username}:${username}" "/tmp/${d}" || exit 1
	done
	for d in runtime cache config ; do
		buildahsetx run --user root "${id}" -- chown -R "${username}:${username}" "/tmp/xdg-${d}-dir" || exit 1
	done
}

function copy_config_files_to_image() {
	local id
	id=$1
	echo "Copying configuration files into working container '${id}'"

	cat <<EOF >"${TEMPDIR}/etc-fonts-local.conf"
<?xml version='1.0' encoding='UTF-8'?>
<!DOCTYPE fontconfig SYSTEM 'fonts.dtd'>
<fontconfig>
 <!-- Generic name aliasing -->
 <alias>
  <family>sans-serif</family>
  <prefer>
   <family>IBM Plex Sans</family>
  </prefer>
 </alias>
 <alias>
  <family>serif</family>
  <prefer>
   <family>IBM Plex Serif</family>
  </prefer>
 </alias>
 <alias>
  <family>monospace</family>
  <prefer>
   <family>IBM Plex Mono</family>
  </prefer>
 </alias>
</fontconfig>
EOF
	buildahsetx copy "${id}" "${TEMPDIR}/etc-fonts-local.conf" /etc/fonts/local.conf || exit 1

	# Make KDE use double-click
	cat <<EOF >"${TEMPDIR}/kbibtex-kdeglobals"
[General]
ColorScheme=Breeze
Name=Breeze
widgetStyle=Breeze

[Icons]
Theme=breeze

[KDE]
SingleClick=false
ColorScheme=Breeze
LookAndFeelPackage=org.kde.breeze.desktop
widgetStyle=breeze
EOF
	buildahsetx copy "${id}" "${TEMPDIR}/kbibtex-kdeglobals" /tmp/xdg-config-home/kdeglobals || exit 1
}


function create_ubuntu_ddebslist() {
	local CODENAME
	CODENAME="$1"

	{
		echo "deb http://ddebs.ubuntu.com ${CODENAME} main restricted universe multiverse"
		echo "deb http://ddebs.ubuntu.com ${CODENAME}-updates main restricted universe multiverse"
		echo "deb http://ddebs.ubuntu.com ${CODENAME}-proposed main restricted universe multiverse"
	} >"${TEMPDIR}/ddebs.list"
}


function build_archlinux() {
	local FROMIMAGE
	FROMIMAGE="docker://archlinux:latest"
	local IMAGENAME
	IMAGENAME="archlinux-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildah rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podman rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	buildahsetx run --user root "${id}" -- pacman -Syu --noconfirm || exit 1
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- pacman -S --noconfirm cmake gcc make extra-cmake-modules poppler-qt5 qt5-xmlpatterns qt5-networkauth qt5-webengine ki18n kxmlgui kio kiconthemes kparts kcoreaddons kservice kwallet kcrash kdoctools ktexteditor breeze-icons frameworkintegration gdb xdg-desktop-portal-kde git gettext ttf-ibm-plex sudo okular appstream || exit 1
	buildahsetx run --user root "${id}" -- rm -f /var/cache/pacman/pkg/*.pkg* || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


function build_debian10() {
	local FROMIMAGE
	FROMIMAGE="docker://debian:buster"
	local IMAGENAME
	IMAGENAME="debian10-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildah rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podman rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	echo "deb http://ftp.se.debian.org/debian/ buster main contrib" >"${TEMPDIR}/etc-apt-sources.list"
	buildahsetx copy "${id}" "${TEMPDIR}/etc-apt-sources.list" /etc/apt/sources.list || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	buildahsetx run --user root "${id}" -- apt dist-upgrade -y || exit 1
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- apt install -y sudo fonts-ibm-plex cmake g++ make extra-cmake-modules libicu-dev libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev libqt5webenginewidgets5 qtwebengine5-dev libqt5webchannel5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme kde-style-breeze frameworkintegration gdb xdg-desktop-portal-kde git gettext okular appstream || exit 1
	buildahsetx run --user root "${id}" -- apt -y clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


function build_debian11() {
	local FROMIMAGE
	FROMIMAGE="docker://debian:bullseye"
	local IMAGENAME
	IMAGENAME="debian11-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildah rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podman rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	echo "deb http://ftp.se.debian.org/debian/ bullseye main contrib" >"${TEMPDIR}/etc-apt-sources.list"
	buildahsetx copy "${id}" "${TEMPDIR}/etc-apt-sources.list" /etc/apt/sources.list || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	buildahsetx run --user root "${id}" -- apt -y full-upgrade || exit 1
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- apt install -y sudo fonts-ibm-plex cmake g++ make extra-cmake-modules libicu-dev libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev libqt5webenginewidgets5 qtwebengine5-dev libqt5webchannel5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme kde-style-breeze frameworkintegration gdb xdg-desktop-portal-kde git gettext okular appstream || exit 1
	buildahsetx run --user root "${id}" -- apt -y clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


function build_debian12() {
	local FROMIMAGE
	FROMIMAGE="docker://debian:bookworm"
	local IMAGENAME
	IMAGENAME="debian12-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	echo "deb http://ftp.se.debian.org/debian/ bookworm main contrib" >"${TEMPDIR}/etc-apt-sources.list"
	buildahsetx copy "${id}" "${TEMPDIR}/etc-apt-sources.list" /etc/apt/sources.list || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	buildahsetx run --user root "${id}" -- apt -y full-upgrade || exit 1
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- apt install -y sudo fonts-ibm-plex cmake g++ make extra-cmake-modules libicu-dev libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev libqt5webenginewidgets5 qtwebengine5-dev libqt5webchannel5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme kde-style-breeze frameworkintegration gdb xdg-desktop-portal-kde git gettext okular appstream || exit 1
	buildahsetx run --user root "${id}" -- apt -y clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


function build_kdeneon() {
	local FROMIMAGE
	FROMIMAGE="invent-registry.kde.org/neon/docker-images/plasma:unstable"
	local IMAGENAME
	IMAGENAME="kdeneon-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildah rm "${WORKINGCONTAINERNAME}" #2>/dev/null >&2
	podman rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	buildahsetx run --user root "${id}" -- apt install -y ubuntu-dbgsym-keyring || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	buildahsetx run --user root "${id}" -- apt -y full-upgrade || exit 1
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- apt -y install sudo fonts-ibm-plex cmake g++ make libicu-dev qt6-networkauth-dev libpoppler-qt6-dev qt6-base-dev-tools kf6-syntax-highlighting-dev kf6-extra-cmake-modules kf6-syntax-highlighting-dev kf6-ki18n-dev kf6-kxmlgui-dev kf6-kiconthemes-dev kf6-kparts-dev kf6-kcoreaddons-dev kf6-kservice-dev kf6-ktextwidgets-dev kf6-kwallet-dev kf6-kcrash-dev kf6-kdoctools-dev kf6-ktexteditor-dev kf6-breeze-icon-theme gdb valgrind git gettext || exit 1
	buildahsetx run --user root "${id}" -- apt -y clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" neon || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


function build_fedora() {
	local variant="${1:-latest}"
	local FROMIMAGE
	FROMIMAGE="docker://fedora:${variant}"
	local IMAGENAME
	IMAGENAME="fedora-${variant}-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildah rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podman rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- dnf install -y cmake g++ make extra-cmake-modules libicu-devel poppler-qt5-devel qt5-qtxmlpatterns-devel qt5-qtnetworkauth-devel qt5-qtwebengine-devel kf5-ki18n-devel kf5-kxmlgui-devel kf5-kio-devel kf5-kiconthemes-devel kf5-kparts-devel kf5-kcoreaddons-devel kf5-kservice-devel kf5-kwallet-devel kf5-kcrash-devel kf5-kdoctools-devel kf5-ktexteditor-devel breeze-icon-theme kf5-frameworkintegration gdb xdg-desktop-portal-kde git gettext okular appstream bison libevent-devel openssl-devel libretls-devel || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


function build_ubuntu2204() {
	local FROMIMAGE
	FROMIMAGE="docker://ubuntu:22.04"
	local IMAGENAME
	IMAGENAME="ubuntu2204-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	export TZ="Europe/Berlin"
	buildahsetx config --user root --env TZ="${TZ}" "${id}"
	echo "${TZ}" >"${TEMPDIR}/timezone"
	buildahsetx copy "${id}" "${TEMPDIR}/timezone" /etc/timezone || exit 1
	buildahsetx run --user root "${id}" -- ln -snf /usr/share/zoneinfo/"${TZ}" /etc/localtime || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	create_ubuntu_ddebslist jammy
	buildahsetx copy "${id}" "${TEMPDIR}/ddebs.list" /etc/apt/sources.list.d/ddebs.list || exit 1
	buildahsetx run --user root "${id}" -- apt install -y ubuntu-dbgsym-keyring || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	buildahsetx run --user root "${id}" -- apt -y full-upgrade || exit 1
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- apt install -y sudo fonts-ibm-plex cmake g++ make extra-cmake-modules libicu-dev libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev libqt5webenginewidgets5 qtwebengine5-dev libqt5webchannel5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme kde-style-breeze frameworkintegration gdb valgrind xdg-desktop-portal-kde git gettext okular appstream || exit 1
	buildahsetx run --user root "${id}" -- apt -y clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


function build_ubuntu2210() {
	local FROMIMAGE
	FROMIMAGE="docker://ubuntu:22.10"
	local IMAGENAME
	IMAGENAME="ubuntu2210-kde-devel"
	local WORKINGCONTAINERNAME
	WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories "${id}"
	set_environment "${id}"

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	export TZ="Europe/Berlin"
	buildahsetx config --user root --env TZ="${TZ}" "${id}"
	echo "${TZ}" >"${TEMPDIR}/timezone"
	buildahsetx copy "${id}" "${TEMPDIR}/timezone" /etc/timezone || exit 1
	buildahsetx run --user root "${id}" -- ln -snf /usr/share/zoneinfo/"${TZ}" /etc/localtime || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	create_ubuntu_ddebslist kinetic
	buildahsetx copy "${id}" "${TEMPDIR}/ddebs.list" /etc/apt/sources.list.d/ddebs.list || exit 1
	buildahsetx run --user root "${id}" -- apt install -y ubuntu-dbgsym-keyring || exit 1
	buildahsetx run --user root "${id}" -- apt update || exit 1
	buildahsetx run --user root "${id}" -- apt -y full-upgrade || exit 1
	# TODO install BibUtils
	buildahsetx run --user root "${id}" -- apt install -y sudo fonts-ibm-plex cmake g++ make extra-cmake-modules libicu-dev libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev libqt5webenginewidgets5 qtwebengine5-dev libqt5webchannel5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme kde-style-breeze frameworkintegration gdb valgrind xdg-desktop-portal-kde git gettext okular appstream || exit 1
	buildahsetx run --user root "${id}" -- apt -y clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" "${IMAGENAME}" 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"

	return 0
}


if (( $# == 1 )) ; then
	if [[ $1 == "--cleanup" ]] ; then
		buildahsetx images -f dangling=true
		echo "!!! Remove any with  podman rmi -f IMAGE-HEX"
		echo
		podmansetx image ls
		echo "!!! Remove any with  podman rmi -f IMAGE-HEX"
		echo
		podmansetx container ls
		echo "!!! Remove any with  buildah rm CONTAINER-HEX"
		echo
		buildahsetx containers
		echo "!!! Remove any with  buildah rm CONTAINER-HEX"
		exit 0
	elif [[ $1 == "archlinux" ]] ; then
		build_archlinux || exit 1
	elif [[ $1 == "debian10" || $1 == "buster" ]] ; then
		build_debian10 || exit 1
	elif [[ $1 == "debian11" || $1 == "bullseye" ]] ; then
		build_debian11 || exit 1
	elif [[ $1 == "debian12" || $1 == "bookworm" ]] ; then
		build_debian12 || exit 1
	elif [[ $1 == "fedora" ]] ; then
		build_fedora latest || exit 1
	elif [[ $1 == "fedora3"* || $1 == "fedora4"* ]] ; then
		build_fedora "${1:6}" || exit 1
	elif [[ $1 == "ubuntu2204" ]] ; then
		build_ubuntu2204 || exit 1
	elif [[ $1 == "ubuntu"* ]] ; then
		build_ubuntu2210 || exit 1
	elif [[ $1 == "kdeneon" ]] ; then
		build_kdeneon || exit 1
	else
		echo "Unknown argument, expecting one of the following:  archlinux  debian10  debian11  debian12  fedora  fedora36  fedora37  fedora38  ubuntu2204  ubuntu2210  kdeneon" >&2
		echo "To get help how to clean up previously created images or containers, run  $(basename "$0") --cleanup" >&2
		exit 1
	fi
else
	echo "Missing argument, expecting one of the following:  archlinux  debian10  debian11  debian12  fedora  fedora36  fedora37  fedora38  ubuntu2204  ubuntu2210  kdeneon" >&2
	echo "To get help how to clean up previously created images or containers, run  $(basename "$0") --cleanup" >&2
	exit 1
fi

if [[ -n "${finalimagename}" ]] ; then
	echo "-------------------------------------------------------------------------------"
	echo "To run a container with just Bash, issue:"
	echo "  podman container run --rm --net=host --tty --interactive ${finalimagename} /bin/bash"
	echo "-------------------------------------------------------------------------------"
fi
