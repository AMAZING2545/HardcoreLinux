# GNOME desktop

Obsidian Core Linux uses GNOME as its default desktop direction.

The current profile is designed around the GNOME desktop stack on musl. GNOME 51 is the current upstream stable release as of September 2026. The profile is intentionally described as a package target rather than claiming that every package is already built and runtime-tested on Obsidian Core Linux.

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

Alpine Linux demonstrates that the GNOME stack can be packaged on musl, so the distro is not tied to glibc for this desktop direction. The Obsidian Core Linux goal is to package the required components as native yspkg packages and keep yspm as the only package manager.


## Laptop and workstation integration

The desktop setup also prepares the system for normal laptop use:

- NetworkManager with iwd support
- PipeWire and WirePlumber
- Bluetooth through BlueZ
- UPower and power-profiles-daemon
- UDisks for removable storage
- CUPS for printing
- fwupd for firmware management
- chrony for time synchronization
- nftables for a default host firewall

These services are configured through the native Hardcore Linux init/service layer rather than systemd.

## Recovery

At boot, adding:

~~~text
obsidian.recovery=1
~~~

starts the recovery shell instead of the normal login path.

Useful commands include:

~~~bash
hc-doctor
yspm check
yspm history
initctl list
~~~
