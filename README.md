# Obsidian Core Linux

Obsidian Core Linux is an independent Linux distribution project built around musl, BusyBox, a small native init/service layer, and yspm.

## Project direction

Obsidian Core Linux aims to be a complete general-purpose desktop distribution rather than only a bootstrap environment.

The default desktop is GNOME. The system keeps a lightweight fallback profile for lower-resource machines.

The project uses:

- musl libc
- BusyBox
- a small auditable init and service layer
- yspm as the only package manager
- UEFI x86_64 boot support
- GNOME as the default desktop
- GDM as the preferred graphical login manager
- PipeWire and WirePlumber
- NetworkManager
- Bluetooth support
- a native .yspkg repository

## Current development line

The current development work covers:

- interactive installer
- guided GPT or manual partitioning
- root and user account creation
- filesystem and bootloader setup
- native yspm bootstrap
- GNOME desktop profile
- graphical login integration
- audio, network, Bluetooth and desktop portal foundations
- live ISO generation
- UEFI disk image generation
- native package build recipes
- legacy package migration
- repository indexes and checksums
- CI validation and tagged releases
- installation and troubleshooting documentation

## yspm

yspm is the only package manager used by the distribution.

Typical commands:

~~~bash
sudo yspm update
sudo yspm search gnome
sudo yspm install gnome-shell
sudo yspm upgrade
~~~

The distribution does not maintain a second package manager.

## Build

Build yspm:

~~~bash
sh tools/build-yspm
~~~

Build the root filesystem:

~~~bash
sudo env YSPM_BIN=/tmp/yspm YSPM_ARCHIVE_DIR="$PWD/repo" sh build/rootfs.build
~~~

Build a UEFI disk image:

~~~bash
sudo sh tools/extract-kernel repo/linux_UEFI_standalone.tar /tmp/vmlinuz
sudo sh tools/build-image build/rootfs.tar /tmp/vmlinuz ObsidianCoreLinux.img 2G
~~~

Build a live ISO:

~~~bash
sudo sh tools/build-iso build/rootfs.tar /tmp/vmlinuz ObsidianCoreLinux.iso
~~~

## Installation

Boot the live ISO in UEFI x86_64 mode.

The live environment launches the interactive installer on tty1. It supports guided GPT partitioning or manual partitions, configures the first user, writes filesystem mounts, installs the bootloader, and removes the live-only marker from the installed system.

See docs/INSTALL.md.

## Desktop

GNOME is the default desktop direction.

The target desktop profile includes the GNOME shell, Mutter, session components, settings, control center, file manager, portals, storage integration, permissions, login manager, audio, networking and Bluetooth.

See docs/GNOME.md.

## Native repository

Native system packages use the .yspkg format.

The package format provides metadata, file ownership, SHA-256 verification, shared-library dependency information, configuration-file handling, lifecycle hooks, service integration, triggers and transaction state through yspm.

## Security and integrity

Release images and repository metadata receive SHA-256 checksums.

Repository indexes can additionally use Ed25519 signatures supported by yspm.

## CI/CD

Pull requests and pushes validate shell syntax, ShellCheck and Python syntax.

Version tags build:

- a UEFI disk image
- a live ISO
- the native package repository
- repository metadata
- SHA-256 checksums

## Development status

The distribution is still under active development.

The largest remaining milestone is populating and runtime-testing the complete native GNOME package stack on musl, followed by real ISO boot testing, hardware testing, installer hardening, recovery tooling, firmware coverage and release qualification.

The project intentionally does not publish invented benchmark numbers or pretend that an untested component is production-ready.

## License

GNU General Public License v2.0.