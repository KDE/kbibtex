#!/usr/bin/env bash

set -euo pipefail

export TEMPDIR="$(mktemp --tmpdir -d "$(basename "${0/.sh}")"-XXXXX.d)"
function cleanup_on_exit {
	rm -rf ${TEMPDIR}
}
trap cleanup_on_exit EXIT

function buildahsetx() {
	set -x
	buildah "${@}"
	{ set +x ; } 2>/dev/null
}

function podmansetx() {
	set -x
	podman "${@}"
	{ set +x ; } 2>/dev/null
}

function create_directories() {
	local id=$1

	for d in source build kbibtex-testset xdg-config-home kbibtex-podman ; do
		buildahsetx run ${id} -- mkdir -p "/tmp/${d}" || exit 1
	done
	for d in runtime cache config ; do
		buildahsetx run ${id} -- mkdir -m 700 -p "/tmp/xdg-${d}-dir" || exit 1
	done
}

function create_user() {
	local OPTIND o sudocmd
	while getopts ":s" o ; do
		[[ ${o} = "s" ]] && sudocmd=sudo
	done
	shift $((OPTIND-1))

	local id=$1
	local username="${2:-kdeuser}"

	echo "Creating user '${username}' in working container '${id}'"

	[[ ${username} != "neon" ]] && buildahsetx run ${id} -- ${sudocmd} useradd --home-dir /tmp --no-log-init --no-create-home --shell /bin/bash --user-group ${username}
	for d in source build kbibtex-testset xdg-config-home ; do
		buildahsetx run ${id} -- ${sudocmd} chown -R ${username}:${username} "/tmp/${d}" || exit 1
	done
	for d in runtime cache config ; do
		buildahsetx run ${id} -- ${sudocmd} chown -R ${username}:${username} "/tmp/xdg-${d}-dir" || exit 1
	done
}

function copy_config_files_to_image() {
	local id=$1
	echo "Copying configuration files into working container '${id}'"

	cat <<EOF >${TEMPDIR}/etc-fonts-local.conf
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
	buildahsetx copy ${id} ${TEMPDIR}/etc-fonts-local.conf /etc/fonts/local.conf || exit 1

	# Make KDE use double-click
	cat <<EOF >${TEMPDIR}/kbibtex-kdeglobals
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
	buildahsetx copy ${id} ${TEMPDIR}/kbibtex-kdeglobals /tmp/xdg-config-home/kdeglobals || exit 1
}


function build_archlinux() {
	local FROMIMAGE="docker://archlinux:latest"
	local IMAGENAME="archlinux-kde-devel"
	local WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories ${id}

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	buildahsetx run ${id} -- pacman -Syu --noconfirm || exit 1
	buildahsetx run ${id} -- pacman -S --noconfirm cmake gcc make extra-cmake-modules poppler-qt5 qt5-xmlpatterns qt5-networkauth qt5-webengine ki18n kxmlgui kio kiconthemes kparts kcoreaddons kservice kwallet kcrash kdoctools ktexteditor breeze-icons frameworkintegration xdg-desktop-portal-kde git gettext ttf-ibm-plex sudo okular || exit 1
	buildahsetx run ${id} -- rm -f /var/cache/pacman/pkg/*.pkg* || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image ${id}
	create_user ${id}

	buildahsetx commit ${id} ${IMAGENAME} || exit 1
}


function build_debian10() {
	local FROMIMAGE="docker://debian:buster"
	local IMAGENAME="debian10-kde-devel"
	local WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories ${id}

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	echo "deb http://ftp.se.debian.org/debian/ buster main contrib" >${TEMPDIR}/etc-apt-sources.list
	buildahsetx copy ${id} ${TEMPDIR}/etc-apt-sources.list /etc/apt/sources.list || exit 1
	buildahsetx run ${id} -- apt update || exit 1
	buildahsetx run ${id} -- apt install -y sudo fonts-ibm-plex cmake g++ make extra-cmake-modules libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev libqt5webenginewidgets5 qtwebengine5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme kde-style-breeze frameworkintegration xdg-desktop-portal-kde git gettext okular || exit 1
	buildahsetx run ${id} -- sudo apt autoremove || exit 1
	buildahsetx run ${id} -- sudo apt clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image ${id}
	create_user ${id}

	buildahsetx commit ${id} ${IMAGENAME} || exit 1
}


function build_debian11() {
	local FROMIMAGE="docker://debian:bullseye"
	local IMAGENAME="debian11-kde-devel"
	local WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories ${id}

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	echo "deb http://ftp.se.debian.org/debian/ bullseye main contrib" >${TEMPDIR}/etc-apt-sources.list
	buildahsetx copy $id ${TEMPDIR}/etc-apt-sources.list /etc/apt/sources.list || exit 1
	buildahsetx run ${id} -- apt update || exit 1
	buildahsetx run ${id} -- apt install -y sudo fonts-ibm-plex cmake g++ make extra-cmake-modules libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev libqt5webenginewidgets5 qtwebengine5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme kde-style-breeze frameworkintegration xdg-desktop-portal-kde git gettext okular || exit 1
	buildahsetx run ${id} -- sudo apt autoremove || exit 1
	buildahsetx run ${id} -- sudo apt clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image ${id}
	create_user ${id}

	buildahsetx commit ${id} ${IMAGENAME} || exit 1
}


function build_kdeneon() {
	local FROMIMAGE="docker://kdeneon/plasma:developer"
	local IMAGENAME="kdeneon-kde-devel"
	local WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories ${id}

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	# Actually require 'sudo' here, no valid root on kdeneon
	buildahsetx run ${id} -- sudo apt clean || exit 1
	buildahsetx run ${id} -- sudo apt update || exit 1
	buildahsetx run ${id} -- sudo apt dist-upgrade -y || exit 1 # typically up-to-date, but still possible: 
	buildahsetx run ${id} -- sudo apt install -y cmake extra-cmake-modules libpoppler-qt5-dev libqt5xmlpatterns5-dev libqt5networkauth5-dev qtwebengine5-dev libqt5webchannel5-dev libkf5i18n-dev libkf5xmlgui-dev libkf5kio-dev libkf5iconthemes-dev libkf5parts-dev libkf5coreaddons-dev libkf5service-dev libkf5wallet-dev libkf5crash-dev libkf5doctools-dev libkf5texteditor-dev breeze-icon-theme git gettext okular || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image ${id}
	create_user -s ${id} neon

	buildahsetx commit ${id} ${IMAGENAME} || exit 1
}


function build_fedora() {
	local FROMIMAGE="docker://fedora:latest"
	local IMAGENAME="fedora-kde-devel"
	local WORKINGCONTAINERNAME="working-$(sed -r 's!docker://!!g;s![^a-z0-9.-]+!-!g' <<<"${FROMIMAGE}")"

	# Remove any residual images of the same name, ignore errors such as if no such image
	buildahsetx rm "${WORKINGCONTAINERNAME}" 2>/dev/null >&2
	podmansetx rmi -f "${IMAGENAME}" 2>/dev/null >&2

	# Pull base image from remote repository
	id=$(buildahsetx from --name "${WORKINGCONTAINERNAME}" "${FROMIMAGE}") || exit 1
	[[ -n "${id}" ]] || { echo "ID is empty" >&2 ; exit 1 ; }
	echo "Using ID ${id} for '${IMAGENAME}'"

	create_directories ${id}

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	buildahsetx run ${id} -- dnf install -y cmake g++ make extra-cmake-modules poppler-qt5-devel qt5-qtxmlpatterns-devel qt5-qtnetworkauth-devel qt5-qtwebengine-devel kf5-ki18n-devel kf5-kxmlgui-devel kf5-kio-devel kf5-kiconthemes-devel kf5-kparts-devel kf5-kcoreaddons-devel kf5-kservice-devel kf5-kwallet-devel kf5-kcrash-devel kf5-kdoctools-devel kf5-ktexteditor-devel breeze-icon-theme kf5-frameworkintegration xdg-desktop-portal-kde git gettext okular bison libevent-devel openssl-devel libretls-devel || exit 1
	buildahsetx run ${id} -- "cd /tmp && git clone 'https://git.omarpolo.com/gmid'"
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image ${id}
	create_user ${id}

	buildahsetx commit ${id} ${IMAGENAME} || exit 1
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
	elif [[ $1 == "fedora" ]] ; then
		build_fedora || exit 1
	elif [[ $1 == "kdeneon" ]] ; then
		build_kdeneon || exit 1
	else
        echo "Unknown argument, expecting one of the following:  archlinux  debian10  debian11  fedora  kdeneon" >&2
        echo "To get help how to clean up previously created images or containers, run  $(basename $0) --cleanup" >&2
        exit 1
	fi
else
	echo "Missing argument, expecting one of the following:  archlinux  debian10  debian11  fedora  kdeneon" >&2
	echo "To get help how to clean up previously created images or containers, run  $(basename $0) --cleanup" >&2
	exit 1
fi

echo "-------------------------------------------------------------------------------"
echo "To run a container with just Bash, issue:"
echo "  podman container run --net=host -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v ${HOME}/.Xauthority:/.Xauthority -v ${HOME}/git/kbibtex-testset:/tmp/kbibtex-testset --tty --interactive IMAGE-HEX /bin/bash"
echo
echo "Choose one of the images from this list:"
podmansetx image ls
echo "-------------------------------------------------------------------------------"
