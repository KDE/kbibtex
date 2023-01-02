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
	{ set +x ; } 2>/dev/null
}

function podmansetx() {
	set -x
	podman "${@}"
	{ set +x ; } 2>/dev/null
}

function create_directories() {
	local id=$1

	for d in xdg-config-home kbibtex-podman ; do
		buildahsetx run "${id}" -- mkdir -p "/tmp/${d}" || exit 1
	done
	for d in runtime cache config ; do
		buildahsetx run "${id}" -- mkdir -m 700 -p "/tmp/xdg-${d}-dir" || exit 1
	done
}

function create_user() {
	local OPTIND o sudocmd
	sudocmd=""
	while getopts ":s" o ; do
		[[ ${o} = "s" ]] && sudocmd=sudo
	done
	shift $((OPTIND-1))

	local id=$1
	local username="${2:-kdeuser}"

	echo "Creating user '${username}' in working container '${id}'"

	[[ "${username}" != "neon" ]] && buildahsetx run "${id}" -- "${sudocmd}" useradd --home-dir /tmp --no-log-init --no-create-home --shell /bin/bash --user-group "${username}"
	buildahsetx run "${id}" -- "${sudocmd}" chown -R "${username}:${username}" "/tmp/xdg-config-home" || exit 1
	for d in runtime cache config ; do
		buildahsetx run "${id}" -- "${sudocmd}" chown -R "${username}:${username}" "/tmp/xdg-${d}-dir" || exit 1
	done
}

function copy_config_files_to_image() {
	local id=$1
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

	# Make KDE use double-click and use Breeze theme
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
	local CODENAME="$1"

	{
		echo "deb http://ddebs.ubuntu.com ${CODENAME} main restricted universe multiverse"
		echo "deb http://ddebs.ubuntu.com ${CODENAME}-updates main restricted universe multiverse"
		echo "deb http://ddebs.ubuntu.com ${CODENAME}-proposed main restricted universe multiverse"
	} >"${TEMPDIR}/ddebs.list"
}


function build_ubuntu2204() {
	local FROMIMAGE
	FROMIMAGE="docker://ubuntu:22.04"
	local IMAGENAME
	IMAGENAME="ubuntu2204-kbibtex-package"
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

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	create_ubuntu_ddebslist jammy
	buildahsetx copy "${id}" "${TEMPDIR}/ddebs.list" /etc/apt/sources.list.d/ddebs.list || exit 1
	export TZ="Europe/Berlin"
	buildah config --env TZ="${TZ}" "${id}"
	echo "${TZ}" >"${TEMPDIR}/timezone"
	buildahsetx copy "${id}" "${TEMPDIR}/timezone" /etc/timezone || exit 1
	buildahsetx run "${id}" -- ln -snf /usr/share/zoneinfo/"${TZ}" /etc/localtime || exit 1
	buildahsetx run "${id}" -- apt install -y ubuntu-dbgsym-keyring || exit 1
	buildahsetx run "${id}" -- apt update || exit 1
	# TODO install BibUtils
	buildahsetx run "${id}" -- apt install -y sudo fonts-ibm-plex breeze-icon-theme kde-style-breeze frameworkintegration gdb valgrind xdg-desktop-portal-kde okular kbibtex kbibtex-dbgsym || exit 1
	buildahsetx run "${id}" -- sudo apt autoremove || exit 1
	buildahsetx run "${id}" -- sudo apt clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" ${IMAGENAME} 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"
}


function build_ubuntu2210() {
	local FROMIMAGE
	FROMIMAGE="docker://ubuntu:22.10"
	local IMAGENAME
	IMAGENAME="ubuntu2210-kbibtex-package"
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

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	create_ubuntu_ddebslist kinetic
	buildahsetx copy "${id}" "${TEMPDIR}/ddebs.list" /etc/apt/sources.list.d/ddebs.list || exit 1
	export TZ="Europe/Berlin"
	buildah config --env TZ="${TZ}" "${id}"
	echo "${TZ}" >"${TEMPDIR}/timezone"
	buildahsetx copy "${id}" "${TEMPDIR}/timezone" /etc/timezone || exit 1
	buildahsetx run "${id}" -- ln -snf /usr/share/zoneinfo/"${TZ}" /etc/localtime || exit 1
	buildahsetx run "${id}" -- apt install -y ubuntu-dbgsym-keyring || exit 1
	buildahsetx run "${id}" -- apt update || exit 1
	# TODO install BibUtils
	buildahsetx run "${id}" -- apt install -y sudo fonts-ibm-plex breeze-icon-theme kde-style-breeze frameworkintegration gdb valgrind xdg-desktop-portal-kde okular kbibtex kbibtex-dbgsym || exit 1
	buildahsetx run "${id}" -- sudo apt autoremove || exit 1
	buildahsetx run "${id}" -- sudo apt clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" ${IMAGENAME} 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"
}


function build_debian12() {
	local FROMIMAGE
	FROMIMAGE="docker://debian:bookworm"
	local IMAGENAME
	IMAGENAME="debian12-kbibtex-package"
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

	# DISTRIBUTION-SPECIFIC CODE BEGINS HERE
	echo "deb http://ftp.se.debian.org/debian/ bookworm main contrib" >"${TEMPDIR}/etc-apt-sources.list"
	buildahsetx copy "${id}" "${TEMPDIR}/etc-apt-sources.list" /etc/apt/sources.list || exit 1
	buildahsetx run "${id}" -- apt update || exit 1
	buildahsetx run "${id}" -- apt dist-upgrade -y || exit 1 # typically up-to-date, but still possible:
	# TODO install BibUtils
	buildahsetx run "${id}" -- apt install -y sudo fonts-ibm-plex breeze-icon-theme kde-style-breeze frameworkintegration gdb xdg-desktop-portal-kde okular kbibtex || exit 1
	buildahsetx run "${id}" -- apt autoremove || exit 1
	buildahsetx run "${id}" -- apt clean || exit 1
	# DISTRIBUTION-SPECIFIC CODE ENDS HERE

	copy_config_files_to_image "${id}" || exit 1
	create_user "${id}" || exit 1

	buildahsetx commit "${id}" ${IMAGENAME} 2>&1 | tee "${TEMPDIR}/buildah-commit-output.txt" || exit 1
	buildahcommitlastline="$(tail -n 1 <"${TEMPDIR}/buildah-commit-output.txt")"
	grep -qP '^[0-9a-f]{24,96}$' <<<"${buildahcommitlastline}" && finalimagename="${buildahcommitlastline}"
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
	elif [[ $1 == "debian12" || $1 == "bookworm" ]] ; then
		build_debian12 || exit 1
	elif [[ $1 == "ubuntu2204" ]] ; then
		build_ubuntu2204 || exit 1
	elif [[ $1 == "ubuntu"* ]] ; then
		build_ubuntu2210 || exit 1
	else
		echo "Unknown argument, expecting one of the following:  debian12  ubuntu2204  ubuntu2210" >&2
		echo "To get help how to clean up previously created images or containers, run  $(basename "$0") --cleanup" >&2
		exit 1
	fi
else
	echo "Missing argument, expecting one of the following:  debian12  ubuntu2204  ubuntu2210" >&2
	echo "To get help how to clean up previously created images or containers, run  $(basename "$0") --cleanup" >&2
	exit 1
fi

if [[ -n "${finalimagename}" ]] ; then
	echo "-------------------------------------------------------------------------------"
	echo "To run KBibTeX in a container, issue:"
	echo "  xhost local:root && mkdir -p /tmp/kbibtex-podman && podman container run --rm --net=host -e DISPLAY --env XDG_RUNTIME_DIR=/tmp/xdg-runtime-dir --env XDG_CACHE_HOME=/tmp/xdg-cache-home -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --env XDG_CONFIG_HOME=/tmp/xdg-config-home --env XDG_DATA_HOME=/tmp/xdg-data-home --env XDG_CURRENT_DESKTOP=KDE -v /tmp/.X11-unix:/tmp/.X11-unix -v ${HOME}/.Xauthority:/.Xauthority --tty --interactive ${finalimagename} sudo -u kdeuser /usr/bin/kbibtex ; xhost -local:root"
	echo
	echo "To run KBibTeX in a container inside of gdb, issue:"
	echo "  xhost local:root && mkdir -p /tmp/kbibtex-podman && podman container run --rm --net=host -e DISPLAY --env XDG_RUNTIME_DIR=/tmp/xdg-runtime-dir --env XDG_CACHE_HOME=/tmp/xdg-cache-home -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --env XDG_CONFIG_HOME=/tmp/xdg-config-home --env XDG_DATA_HOME=/tmp/xdg-data-home --env XDG_CURRENT_DESKTOP=KDE -v /tmp/.X11-unix:/tmp/.X11-unix -v ${HOME}/.Xauthority:/.Xauthority --tty --interactive ${finalimagename} sudo -u kdeuser gdb /usr/bin/kbibtex ; xhost -local:root"
	echo
	echo "To run a container with just Bash, issue:"
	echo "  podman container run --rm --net=host --tty --interactive ${finalimagename} /bin/bash"
	echo "-------------------------------------------------------------------------------"
fi
