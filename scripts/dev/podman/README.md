# Scripts to Run KBibTeX in a Podman Container

*Podman* is a Linux container solution (operating system-level virtualization) that is an alternative to Docker, but is daemonless by design and allows rootless containers.
It is well supported on Linux distributions like Fedora or Arch.
For further information, please visit Podman's webpage at [podman.io](https://podman.io/).

Before proceeding with this README, please ensure that you have a working Podman installation and that you as a non-priviledged user can pull images from Docker's hub and run container instances.
Please refer to your Linux distribution's manual and Podman's official documentation.

There are two scripts that prepare images for KBibTeX and allow running KBibTeX or any of its automated tests.
The first script prepares a container image that contains all necessary tools and development libraries to compile KBibTeX.
This container image only needs to be rebuilt once in a while (e.g. on a weekly or monthly base).
The second script prepares a container image that contains KBibTeX's sources and compiles KBibTeX for its execution.
This image needs to be rebuilt whenever a new revision of the source code needs to be tested.

## Preparing a Developer Image

The first script is called `create-development-image.sh` which creates a developer image.
Several distributions' base systems are available to choose from.
Simply pass one of the pre-configured distributions are an argument to this script.
Available alternatives are `debian10`, `debian11`, `ubuntu`, `fedora`, `archlinux`, `kdeneon`.

Example invocation:

```
./create-development-image.sh kdeneon
```

Creating such developer instances may download several GiB of data, both for the base image itself and additional packages to be installed in the image under creation.

## Creating Image with KBibTeX and Run Graphical Program

The second script, `compile-and-run-in-container.sh` will prepare a container image where it compiles the current source code.
The generated image contains the graphical KBibTeX program and all automated tests.
By default, first all tests are run and then the graphical client is launched.
Making use of X-forwarding, the graphical client will almost look like a locally installed program (with difference due to different icons, themes, and fonts).

Like the first script, this second script takes one argument: the distribution-specific image created by the first script.

Example invocation:

```
./compile-and-run-in-container.sh kdeneon
```

## Cleaning and Removing Images and Container Instances

Both scripts support a special argument `--cleanup` instead of a distribution name.
Invoking the scripts with this argument will not actually clean up (i.e. remove) anything, but will print out instructions how to remove container images and instances.

To understand what those `podman` and `buildah` invocations do, please have a look at the commands' man pages and other documentation.
