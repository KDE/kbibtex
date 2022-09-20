#!/usr/bin/env bash

set -euo pipefail

function cleanup_on_exit {
	xhost -local:root
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

function create_imagename() {
	local fromimage="$1"
	sed -r 's!([:/]+|localhost|-kde-devel)!!g' <<<"${fromimage}"
}

function prepare_image() {
	local fromimage="$1"
	local imagename="$2"

	buildahsetx rm "working-${imagename}" 2>/dev/null >&2
	echo "podman rmi -f '${imagename}'" >&2
	podmansetx rmi -f "${imagename}" 2>/dev/null >&2
	
	buildahsetx from --name "working-${imagename}" "${fromimage}" | tee /dev/stderr
}

function prepare_environment() {
	local id="$1"

	buildahsetx copy ${id} ../../../ /tmp/source || exit 1
	buildahsetx config --env DISPLAY=${DISPLAY} --env QT_X11_NO_MITSHM=1 --env KDEDIRS=/usr --env XDG_RUNTIME_DIR=/tmp/xdg-runtime-dir --env XDG_CACHE_HOME=/tmp/xdg-cache-home --env XDG_CONFIG_HOME=/tmp/xdg-config-home --env XDG_DATA_DIRS=/usr/share --env XDG_DATA_HOME=/tmp/xdg-data-home --env XDG_CURRENT_DESKTOP=KDE ${id} || exit 1
}

function build_sources() {
	local id="$1"
	local sudocmd="$2"

	buildahsetx config --workingdir /tmp/build ${id} || exit 1
	buildahsetx run ${id} -- ${sudocmd} cmake -DBUILD_TESTING:BOOL=ON -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX:PATH=/usr ../source || exit 1
	buildahsetx run ${id} -- ${sudocmd} make -j$(( $(nproc) - 1 )) || exit 1
}

function install_program() {
	local id="$1"
	local sudocmd="$2"

	buildahsetx run ${id} -- ${sudocmd} make install || exit 1
	buildahsetx run ${id} -- ${sudocmd} kbuildsycoca5 || exit 1
}

function cleaning() {
	local id="$1"
	local sudocmd="$2"

	buildahsetx run ${id} -- ${sudocmd} rm -rf '/tmp/source'
}

function setting_ownerships() {
	local id="$1"
	local username="$2"
	local sudocmd="$3"

	for d in build xdg-config-home ; do
		buildahsetx run ${id} -- ${sudocmd} chown -R ${username}:${username} "/tmp/${d}" || exit 1
	done
	for d in runtime cache config ; do
		echo "buildah run ${id} -- ${sudocmd} chown -R ${username}:${username} '/tmp/xdg-${d}-dir'" >&2
		buildah run ${id} -- ${sudocmd} chown -R ${username}:${username} "/tmp/xdg-${d}-dir" || exit 1
	done
}

function finalizing_image() {
	local id="$1"
	local username="$2"

	buildahsetx config --workingdir /tmp ${id} || exit 1
	buildahsetx config --cmd "sudo -u ${username} /usr/bin/kbibtex" ${id} || exit 1
	buildahsetx commit ${id} ${id}_img || exit 1
}

function run_test_programs() {
	local id="$1"
	local username="$2"

	mkdir -p /tmp/kbibtex-podman
	# FIXME kbibtexfilestest  requires to have testsets files available
	for testbinary in kbibtexnetworkingtest kbibtexiotest kbibtexdatatest ; do
		podmansetx container run --rm --net=host -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --tty --interactive ${id}_img sudo -u ${username} /tmp/build/bin/${testbinary} -platform offscreen 2>&1 | tee -a /tmp/kbibtex-podman/output-${testbinary}.txt || exit 1
	done
}

function run_kbibtex_program() {
	local id="$1"
	local username="$2"

	mkdir -p /tmp/kbibtex-podman
	xhost local:root
	podmansetx container run --rm --net=host -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v ${HOME}/.Xauthority:/.Xauthority -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --tty --interactive ${id}_img sudo -u ${username} /usr/bin/kbibtex 2>&1 | tee -a /tmp/kbibtex-podman/output-kbibtex.txt || exit 1
	xhost -local:root
}

function run_bash() {
	local id="$1"
	local username="$2"

	mkdir -p /tmp/kbibtex-podman
	podmansetx container run --rm --net=host -e DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix -v ${HOME}/.Xauthority:/.Xauthority -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --tty --interactive ${id}_img sudo -u ${username} /bin/bash || exit 1
}

if (( $# == 1 )) ; then
	if [[ $1 == "archlinux" || $1 == "debian10" || $1 == "debian11" || $1 == "fedora" || $1 == "ubuntu" || $1 == "kdeneon" ]] ; then
		SUDOCMD=""
		[[ $1 == "kdeneon" ]] && SUDOCMD="sudo"
		USERNAME="kdeuser"
		[[ $1 == "kdeneon" ]] && USERNAME="neon"

		FROMIMAGE="localhost/${1}-kde-devel"
		IMAGENAME="$(create_imagename "${FROMIMAGE}")"
		ID="$(prepare_image "${FROMIMAGE}" "${IMAGENAME}")"
		prepare_environment "${ID}" "${SUDOCMD}" || exit 1
		build_sources "${ID}" "${SUDOCMD}" || exit 1
		install_program "${ID}" "${SUDOCMD}" || exit 1
		cleaning "${ID}" "${SUDOCMD}" || exit 1
		setting_ownerships "${ID}" "${USERNAME}" "${SUDOCMD}" || exit 1
		finalizing_image "${ID}" "${USERNAME}" "${SUDOCMD}" || exit 1
		run_test_programs "${ID}" "${USERNAME}" "${SUDOCMD}" || exit 1
		run_kbibtex_program "${ID}" "${USERNAME}" "${SUDOCMD}" || exit 1
	elif [[ $1 == "--cleanup" ]] ; then
		podmansetx image ls
		podmansetx container ls
		buildahsetx images -f dangling=true
		buildahsetx containers
		echo
		buildahrmoutput="$(buildah containers | awk '/  [*]  / {printf " "$1} END {print ""}')"
		podmanimagelsoutput="$(podman image ls | awk '/^localhost\// {printf " "$3}')"
		buildahcontainersoutput="$(buildah containers | awk '/  [*]  / {printf " "$3	} END {print ""}')"
		[[ -n "${buildahrmoutput}" || -n "${podmanimagelsoutput}" || -n "${buildahcontainersoutput}" ]] && echo "To clean, run:"
		[[ -n "${buildahrmoutput}" ]] && echo "  buildah rm ${buildahrmoutput}"
		[[ -n "${podmanimagelsoutput}" || -n "${buildahcontainersoutput}" ]] && echo "  podman rmi -f ${podmanimagelsoutput} ${buildahcontainersoutput}"
		echo
		echo "If all fails, run:"
		echo "  sudo rm -rf ~/.local/share/containers ~/.config/containers"
		exit 0
	else
		echo "Unknown argument, expecting one of the following:  archlinux  debian10  debian11  fedora  ubuntu  kdeneon" >&2
		echo "To get help how to clean up previously created images or containers, run  $(basename $0) --cleanup" >&2
		exit 1
	fi
else
	echo "Missing argument, expecting one of the following:  archlinux  debian10  debian11  fedora  ubuntu  kdeneon" >&2
	echo "To get help how to clean up previously created images or containers, run  $(basename $0) --cleanup" >&2
	exit 1
fi
