# Hardcore Linux

A small, independent Linux distribution built around musl, BusyBox, a shell-based init system, and yspm.

## Project direction

Hardcore Linux aims to stay minimal without turning the base system into an unusable proof of concept.

The base system is intentionally small. Graphical environments and larger applications are installed later from the yspm repository.

The project uses:

- musl libc
- BusyBox
- a small shell init system
- service scripts under /system
- yspm for native package management
- UEFI boot support
- Sway as the primary desktop path
- labwc as an alternative desktop path

## What changed

The current development line adds the pieces needed for a usable distribution rather than only a root filesystem:

- interactive installer
- guided or manual partitioning
- GRUB and systemd-boot installation paths
- hostname and user configuration
- automated desktop setup
- Sway and labwc
- optional Ly display manager
- PipeWire and WirePlumber integration
- iwd wireless service
- D-Bus service
- architecture-independent network bootstrap
- native .yspkg package recipes
- yspm bootstrap integration
- legacy package migration into a native yspm repository
- release SHA-256 checksums
- automated release image builds
- reproducible benchmark methodology
- installation and desktop documentation

## yspm

yspm is the package manager used by the distribution.

The base image contains:

~~~text
/usr/bin/yspm
/etc/yspm/env
/etc/profile.d/yspm.sh
/var/lib/yspm
/var/cache/yspm
~~~

After installation:

~~~bash
sudo yspm update
sudo yspm install sway
sudo yspm upgrade
~~~

The distribution does not maintain a second package manager.

The existing tar archive repository is still kept during the migration to native yspkg packages. The release workflow can convert the legacy packages, build a yspm index, and publish the native packages with the release.

## Build the root filesystem

The existing build system keeps the bootstrap artifacts in the repository.

Build the yspm binary:

~~~bash
sh tools/build-yspm
~~~

Then build the root filesystem:

~~~bash
sudo YSPM_BIN=/tmp/yspm sh build/rootfs.build
~~~

Output:

~~~text
build/rootfs.tar
~~~

The rootfs builder no longer depends on the old flashman package manager.

## Build a bootable image

Extract a kernel from the standalone kernel archive:

~~~bash
sudo sh tools/extract-kernel linux_UEFI_standalone.tar /tmp/vmlinuz
~~~

Build a UEFI disk image:

~~~bash
sudo sh tools/build-image \
  build/rootfs.tar \
  /tmp/vmlinuz \
  HardcoreLinux.img \
  2G
~~~

The image contains the root filesystem, kernel, GRUB, and a copy of rootfs.tar so it can act as an installer environment.

## Install

Boot the image on a UEFI x86_64 machine and run:

~~~bash
sh /usr/sbin/hc-installer
~~~

The installer asks for:

- target disk or partitions
- filesystem formatting
- hostname
- first user
- passwords
- yspm repository
- bootloader

See [docs/INSTALL.md](docs/INSTALL.md).

## Desktop setup

After the first boot:

~~~bash
sudo sh /usr/sbin/hc-setup-desktop
~~~

The wizard can install:

~~~text
Sway
labwc
foot
wofi
waybar
ly
PipeWire
WirePlumber
iwd
udev
D-Bus
~~~

See [docs/DESKTOP.md](docs/DESKTOP.md).

## Init and services

The init system is intentionally small and auditable.

System services live under:

~~~text
/system/services
~~~

Boot scripts live under:

~~~text
/system/scripts
~~~

Configured services are enabled through:

~~~text
/system/enabled-services
~~~

The service control command is:

~~~bash
initctl start <service>
initctl stop <service>
initctl restart <service>
initctl enable <service>
initctl disable <service>
initctl info <service>
initctl list
~~~

The init system can start Ly automatically when it is installed; otherwise it falls back to tty gettys.

## Native packages

Native packages use:

~~~text
.yspkg
~~~

The format contains:

~~~text
metadata.json
scripts/
  preinstall
  postinstall
  preremove
  postremove
root/
  usr/
  etc/
  var/
~~~

Package recipes in build.native use yspm to produce native packages.

See [repo/README.md](repo/README.md) and the yspm project for the package metadata contract.

## Binary repository

The repository currently contains legacy binary archives.

The migration tool can create a native yspm repository:

~~~bash
python3 tools/migrate-legacy-repo.py \
  --input . \
  --output repo-yspm/releases/1 \
  --base-url https://example.org/hardcorelinux/releases/1 \
  --yspm /tmp/yspm
~~~

A release can then publish:

~~~text
repo-index.json
*.yspkg
SHA256SUMS
HardcoreLinux-<tag>.img
~~~

## Integrity

Release images and repository indexes receive SHA-256 checksums.

For local release files:

~~~bash
sh tools/hc-release-checksums SHA256SUMS .
~~~

Repository metadata can be signed with the Ed25519 signing functionality provided by yspm.

## CI/CD

Pull requests and pushes run validation for:

- shell syntax
- ShellCheck errors
- Python syntax

Version tags matching v* trigger the release pipeline.

The release workflow:

1. builds yspm
2. builds the root filesystem
3. extracts the kernel
4. creates a UEFI image
5. converts legacy packages to yspkg
6. generates the native repository index
7. publishes packages and the image
8. publishes SHA-256 checksums

## Benchmarks

The repository does not publish invented performance numbers.

Collect local data with:

~~~bash
sudo sh /usr/bin/hc-bench
~~~

See [docs/BENCHMARKS.md](docs/BENCHMARKS.md).

## Documentation

- [Installation](docs/INSTALL.md)
- [Desktop Setup](docs/DESKTOP.md)
- [Benchmarks](docs/BENCHMARKS.md)
- [Showcase](docs/SHOWCASE.md)
- [Native Repository](repo/README.md)

## License

GNU General Public License v2.0.
