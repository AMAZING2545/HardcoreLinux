# GNOME desktop

Hardcore Linux uses GNOME as its default desktop direction.

The current profile is designed around the GNOME desktop stack on musl. GNOME 51 is the current upstream stable release as of September 2026. The profile is intentionally described as a package target rather than claiming that every package is already built and runtime-tested on Hardcore Linux.

## Desktop stack

The profile targets:

- GNOME Shell and Mutter
- GNOME Session
- GNOME Settings Daemon
- GNOME Control Center
- GDM
- D-Bus
- elogind
- polkit
- NetworkManager
- PipeWire and WirePlumber
- Xwayland
- XDG desktop portals
- GVfs
- UDisks
- AccountsService
- GNOME Keyring
- Adwaita theme assets

## Core applications

The default desktop profile also targets a practical workstation set:

- Files (Nautilus)
- GNOME Console
- GNOME Text Editor
- Calculator
- Calendar
- Loupe
- Evince

## Installation

After installing the base system:

~~~bash
sudo /usr/sbin/hc-setup-desktop
~~~

The setup tool installs the profile through yspm, configures the supporting services, and prepares GDM for the next reboot.

## Current packaging status

The distro can describe and configure the GNOME profile now, but the full native package repository still needs to be populated with GNOME libraries, applications, and their build recipes. This is the next major packaging milestone.

Alpine Linux demonstrates that the GNOME stack can be packaged on musl, so the distro is not tied to glibc for this desktop direction. The Hardcore Linux goal is to package the required components as native yspkg packages and keep yspm as the only package manager.
