#!/usr/bin/env bash

set -euo pipefail

TEMPDIR="$(mktemp --tmpdir -d "$(basename "${0/.sh}")"-XXXXX.d)"
export TEMPDIR
function cleanup_on_exit {
	xhost -local:root
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
	
	buildahsetx from --name "working-${imagename}" "${fromimage}" || exit 1
}

function prepare_environment() {
	local id="$1"
	local checkout
	checkout="$2"

	cat <<EOF >"${TEMPDIR}/runtestprograms.sh"
set -x
# FIXME kbibtexfilestest  requires to have testsets files available
/tmp/build/bin/kbibtexnetworkingtest -platform offscreen || exit 1
/tmp/build/bin/kbibtexiotest -platform offscreen || exit 1
/tmp/build/bin/kbibtexdatatest -platform offscreen || exit 1
set +x
EOF
	buildahsetx copy "${id}" "${TEMPDIR}/runtestprograms.sh" /tmp/ || exit 1

	buildahsetx copy "${id}" ../../../ /tmp/source || exit 1
	buildahsetx run --workingdir /tmp/source "${id}" -- git config --global --add safe.directory /tmp/source || exit 1
	if [[ -n "${checkout}" ]] ; then
		buildahsetx run --workingdir /tmp/source "${id}" -- git reset --hard || exit 1
		buildahsetx run --workingdir /tmp/source "${id}" -- git clean -fd || exit 1
		buildahsetx run --workingdir /tmp/source "${id}" -- git checkout "${checkout}" || exit 1
	fi
}

function build_sources() {
	local id="$1"

	buildahsetx config --workingdir /tmp/build "${id}" || exit 1
	buildahsetx config --user root --env VERBOSE=1 "${id}"
	buildahsetx run --user root "${id}" -- /usr/bin/cmake --log-level=VERBOSE -DBUILD_TESTING:BOOL=ON -DCMAKE_BUILD_TYPE=debug -DCMAKE_INSTALL_PREFIX:PATH=/usr ../source || exit 1
	buildahsetx run --user root "${id}" -- /usr/bin/make -j$(( $(nproc) - 1 )) || exit 1
}

function install_program() {
	local id="$1"

	buildahsetx run --user root "${id}" -- make install || exit 1
	buildahsetx run --user root "${id}" -- kbuildsycoca5 || exit 1
}

function cleaning() {
	local id="$1"

	buildahsetx run --user root "${id}" -- rm -rf '/tmp/source'
}

function setting_ownerships() {
	local id="$1"
	local username="$2"

	for d in build xdg-config-home ; do
		buildahsetx run --user root "${id}" -- chown -R "${username}:${username}" "/tmp/${d}" || exit 1
	done
	for d in runtime cache config ; do
		buildahsetx run --user root "${id}" -- chown -R "${username}:${username}" "/tmp/xdg-${d}-dir" || exit 1
	done
}

function finalizing_image() {
	local id="$1"
	local username="$2"

	buildahsetx config --workingdir /tmp "${id}" || exit 1
	buildahsetx config --cmd "/usr/bin/sudo -u \"${username}\" /usr/bin/kbibtex" "${id}" || exit 1
	buildahsetx commit "${id}" "${id}_img" || exit 1
}

function run_test_programs() {
	local id="$1"
	local username="$2"

	mkdir -p /tmp/kbibtex-podman
	rm -f /tmp/kbibtex-podman/output-{kbibtexnetworkingtest,kbibtexiotest,kbibtexdatatest}.txt

	podmansetx container run --net=host -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --tty --interactive "${id}_img" /usr/bin/sudo -u "${username}" /bin/bash /tmp/runtestprograms.sh 2>&1 | tee -a /tmp/kbibtex-podman/output-testprograms.txt || { echo "For log output, see '/tmp/kbibtex-podman/output-testprograms.txt'" >&2 ; exit 1 ; }
}

function run_kbibtex_program() {
	local id="$1"
	local username="$2"

	mkdir -p /tmp/kbibtex-podman
	xhost local:root
	podmansetx container run --rm --net=host -v /tmp/.X11-unix:/tmp/.X11-unix -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --env DISPLAY --tty --interactive "${id}_img" /usr/bin/sudo -u "${username}" /usr/bin/kbibtex 2>&1 | tee /tmp/kbibtex-podman/output-kbibtex.txt || { echo "For log output, see '/tmp/kbibtex-podman/output-kbibtex.txt'" >&2 ; exit 1 ; }
	xhost -local:root
}

function run_bash() {
	local id="$1"
	local username="$2"

	mkdir -p /tmp/kbibtex-podman
	podmansetx container run --rm --net=host -v /tmp/kbibtex-podman:/tmp/kbibtex-podman --tty --interactive "${id}_img" /usr/bin/sudo -u "${username}" /bin/bash || exit 1
}

if (( $# == 1 || $# == 2)) ; then
	if [[ $1 == "archlinux" || $1 == "debian10" || $1 == "debian11" || $1 == "debian12" || $1 == "fedora" || $1 == "ubuntu"* || $1 == "kdeneon" ]] ; then
		DIST="$1"
		[[ ${DIST} == "ubuntu" ]] && DIST="ubuntu2210"
		USERNAME="kdeuser"
		[[ ${DIST} == "kdeneon" ]] && USERNAME="neon"

		FROMIMAGE="localhost/${DIST}-kde-devel"
		IMAGENAME="$(create_imagename "${FROMIMAGE}")"
		ID="$(prepare_image "${FROMIMAGE}" "${IMAGENAME}")"
		prepare_environment "${ID}" "${2:-}" || exit 1
		build_sources "${ID}" || exit 1
		install_program "${ID}" || exit 1
		cleaning "${ID}" || exit 1
		setting_ownerships "${ID}" "${USERNAME}" || exit 1
		finalizing_image "${ID}" "${USERNAME}" || exit 1
		run_test_programs "${ID}" "${USERNAME}" || exit 1
		run_kbibtex_program "${ID}" "${USERNAME}" || exit 1
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
		echo "Unknown argument, expecting one of the following:  archlinux  debian10  debian11  debian12  fedora  ubuntu2204  ubuntu2210  kdeneon" >&2
		echo "To get help how to clean up previously created images or containers, run  $(basename "$0") --cleanup" >&2
		exit 1
	fi
else
	echo "Missing argument, expecting one of the following:  archlinux  debian10  debian11  debian12  fedora  ubuntu2204  ubuntu2210  kdeneon" >&2
	echo "To get help how to clean up previously created images or containers, run  $(basename "$0") --cleanup" >&2
	exit 1
fi
