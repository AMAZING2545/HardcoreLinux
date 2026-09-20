# FAQ and Troubleshooting

## Is yspm the package manager?

Yes.

Hardcore Linux uses yspm for native package management. The old flashman files remain only as legacy repository/build material during migration and are not installed by the new rootfs builder.

## Why does the base system have no desktop?

The base image is intentionally small.

Install the graphical stack after installation:

~~~bash
sudo sh /usr/sbin/hc-setup-desktop
~~~

Choose Sway or labwc and optionally install Ly, PipeWire, WirePlumber, and iwd.

## Why is a package unavailable?

The native yspm repository is still being built.

Check the configured repository:

~~~bash
yspm release
yspm update
yspm search package-name
~~~

A package recipe can exist in `build.native/` before a binary build is published.

## Why does an application fail on musl?

Hardcore Linux uses musl instead of glibc.

Some software assumes glibc-specific interfaces. Prefer a native musl build when available.

For a package that needs additional compatibility support, document the requirement in its package metadata instead of silently bundling a different libc.

## Network does not start

The base boot script brings up Ethernet-style interfaces and starts BusyBox DHCP.

Check interfaces:

~~~bash
ip link
~~~

For Wi-Fi, install and enable iwd:

~~~bash
sudo yspm install iwd
sudo initctl enable iwd
sudo initctl start iwd
~~~

The installer and base image do not hardcode an interface such as `eth0`.

## Audio does not work

Install the optional audio stack:

~~~bash
sudo yspm install pipewire wireplumber
~~~

Then log in again.

Check the processes:

~~~bash
ps | grep -E 'pipewire|wireplumber'
~~~

The desktop helper also creates an XDG runtime directory when needed.

## Sway needs udev

For input devices, the desktop setup includes udev.

Check:

~~~bash
command -v udevd
~~~

On systems using the included service model, keep udev and its dependencies installed before starting a graphical session.

## Ly does not start

Ly is optional.

The custom init starts Ly only when the `ly` command is installed and the system is not running the live installer.

Without Ly, Hardcore Linux falls back to tty gettys.

## The installer cannot find the kernel

The default kernel path is:

~~~text
/boot/vmlinuz
~~~

The live image builder places the selected kernel there.

You can choose another path when launching the installer.

## I selected the wrong disk

Stop before confirming guided partitioning.

Guided mode intentionally requires an explicit confirmation before it repartitions the selected disk.

Manual mode is available for existing partitions.

## Is the installer UEFI-only?

The current installer and image builder target x86_64 UEFI systems.

Other firmware modes and architectures require additional installer/image work.

## Can the root filesystem be tested without installing?

Yes.

Build the image and run it with QEMU:

~~~bash
sudo sh tools/build-image build/rootfs.tar /tmp/vmlinuz HardcoreLinux.img 2G
sh tools/run-qemu HardcoreLinux.img
~~~

The QEMU helper expects OVMF firmware and KVM.

## Where are package state and cache stored?

System mode uses:

~~~text
/var/lib/yspm
/var/cache/yspm
~~~

The repository configuration is:

~~~text
/etc/yspm/env
~~~

## How do I verify a release image?

Use the published SHA-256 file:

~~~bash
sha256sum -c SHA256SUMS
~~~

For locally generated artifacts:

~~~bash
sh tools/hc-release-checksums
~~~

## Where can I report a bug?

Use the project's issue tracker:

https://github.com/Yassine-Jemi01/HardcoreLinux/issues
