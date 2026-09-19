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

## Current development foundation

The current development line adds the pieces needed for a usable distribution:

- interactive installer
- guided GPT or manual partitioning
- GRUB and systemd-boot installation paths when their tools are available
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
- installation, desktop, and troubleshooting documentation

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

The existing tar archive repository is still kept during the migration to native yspkg packages. The release workflow converts legacy packages, builds a yspm index, and publishes native packages with each release.

## Build the root filesystem

The bootstrap archives currently live under repo/.

Build the yspm binary:

~~~bash
sh tools/build-yspm
~~~

Then build the root filesystem:

~~~bash
sudo env YSPM_BIN=/tmp/yspm YSPM_ARCHIVE_DIR="$PWD/repo" sh build/rootfs.build
~~~

Output:

~~~text
build/rootfs.tar
~~~

The rootfs builder no longer installs the old flashman package manager.

## Build a bootable image

Extract a kernel from the repository's standalone kernel archive:

~~~bash
sudo sh tools/extract-kernel repo/linux_UEFI_standalone.tar /tmp/vmlinuz
~~~

Build a UEFI disk image:

~~~bash
sudo sh tools/build-image \
  build/rootfs.tar \
  /tmp/vmlinuz \
  HardcoreLinux.img \
  2G
~~~

The image contains the root filesystem, kernel, GRUB boot files, and a copy of rootfs.tar so it can act as an installer environment.

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

Guided partitioning uses GPT with a 512 MiB EFI System Partition and the remaining space for an ext4 root filesystem.

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

Package recipes in build.native/ use yspm to produce native packages.

## Binary repository

The repository currently contains a large legacy binary collection under repo/.

The migration tool can create a native yspm repository:

~~~bash
python3 tools/migrate-legacy-repo.py \
  --input repo \
  --output repo-yspm/releases/1 \
  --base-url https://example.org/hardcorelinux/releases/1 \
  --yspm /tmp/yspm
~~~

A release can publish:

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
3. extracts the repository kernel archive
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

## Troubleshooting

See [FAQ and Troubleshooting](docs/FAQ.md) for installer, musl, network, audio, yspm, and desktop issues.

## Documentation

- [Installation](docs/INSTALL.md)
- [Desktop Setup](docs/DESKTOP.md)
- [FAQ and Troubleshooting](docs/FAQ.md)
- [Benchmarks](docs/BENCHMARKS.md)
- [Showcase](docs/SHOWCASE.md)
- [Native Repository](repo/README.md)

## License

GNU General Public License v2.0.
