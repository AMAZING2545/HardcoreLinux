# Installing Hardcore Linux

Hardcore Linux includes a live ISO and a small interactive installer for UEFI x86_64 systems.

The installer is intentionally simple and explicit. It shows the detected disks before any destructive operation and asks for every important system setting.

## Booting the live ISO

The release ISO is the normal starting point for a fresh installation. Boot it in UEFI mode.

The live system starts the installer on tty1. You can also launch it manually with `sh /usr/sbin/hc-installer`.

The current live ISO uses an initramfs-based live root, which keeps the first implementation simple. A compressed read-only live filesystem is planned for later releases.

## Before installing

Back up important data.

You need:

- a booted Hardcore Linux live environment
- a root shell
- an EFI System Partition
- a Linux root partition
- the generated rootfs.tar
- a kernel image

## Start the installer

From the live environment:

~~~bash
sh /usr/sbin/hc-installer
~~~

The installer asks for:

- installation mode
- target disk or partitions
- root filesystem formatting
- rootfs archive
- kernel image
- hostname
- username
- yspm repository
- bootloader

## Guided partitioning

Guided mode creates:

~~~text
GPT
├── 512 MiB EFI System Partition
└── remaining space Linux root
~~~

The root filesystem is ext4.

The disk is erased only after an explicit confirmation.

## Manual partitioning

Manual mode accepts an existing EFI partition and root partition.

The root partition may optionally be formatted as ext4.

## Bootloaders

GRUB is supported for x86_64 UEFI.

systemd-boot is supported when bootctl is available in the live environment. systemd-boot is only the bootloader; Hardcore Linux continues to use its own /etc/init.

## User and root accounts

The installer creates the first normal user and asks for passwords for both that user and root.

The live-only /etc/hc-live marker is removed from the installed system.

## yspm

The installer writes:

~~~text
/etc/yspm/env
/etc/profile.d/yspm.sh
~~~

The selected repository is exported through YSPM_REPOSITORY.

After installation:

~~~bash
sudo yspm update
sudo yspm install sway
sudo yspm upgrade
~~~

No second package manager is needed.

## After first boot

Run:

~~~bash
sudo sh /usr/sbin/hc-setup-desktop
~~~

GNOME is the default desktop profile. The setup installs GNOME, GDM, PipeWire/WirePlumber, NetworkManager, Bluetooth integration, desktop portals, and the supporting services from the yspm repository.
