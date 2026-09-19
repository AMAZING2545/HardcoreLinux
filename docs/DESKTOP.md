# Desktop Setup

Hardcore Linux keeps the base system small and installs graphical software through yspm.

Run:

~~~bash
sudo sh /usr/sbin/hc-setup-desktop
~~~

Choose one of:

- Sway
- labwc

The setup can also install:

- foot
- wofi
- waybar
- ly
- PipeWire
- WirePlumber
- iwd
- udev
- D-Bus

## Sway

The generated configuration provides simple defaults for:

- terminal
- application launcher
- status bar
- exiting the compositor

Users can replace the configuration without changing the base system.

## labwc

labwc is a wlroots-based stacking compositor inspired by Openbox.

It is provided as a second graphical path so the base system is not tied to one compositor.

## Audio

PipeWire and WirePlumber are started for logged-in users when installed.

The session setup creates XDG_RUNTIME_DIR when necessary.

## Wireless

iwd is managed as a native Hardcore Linux service.

Use:

~~~bash
sudo initctl enable iwd
sudo initctl start iwd
~~~

Wi-Fi credentials remain user configuration rather than being hardcoded into the image.

## Display manager

Ly is optional.

The custom init detects ly and starts it instead of the traditional getty loop when the package is installed.
