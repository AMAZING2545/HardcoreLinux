# Benchmarks

Performance claims in the project should come from measurements on the same hardware or the same QEMU configuration.

This repository does not publish invented benchmark numbers.

## System snapshot

Run:

~~~bash
sudo sh /usr/bin/hc-bench
~~~

The command reports:

- kernel and architecture
- uptime
- memory usage
- root filesystem usage
- process count

## Memory

Record the same values after the same services and desktop session have started.

Useful fields are:

~~~text
MemTotal
MemAvailable
MemFree
process count
enabled services
desktop/session
~~~

## Boot

Use the same firmware, image, kernel configuration, storage or QEMU configuration.

Record the time from firmware handoff to a usable shell or desktop.

Also record:

~~~text
commit
kernel version
CPU
RAM
bootloader
enabled services
desktop/session
~~~

## Filesystem and image size

Before compression:

~~~bash
du -sh build/rootfs
~~~

For release artifacts:

~~~bash
ls -lh *.img *.iso 2>/dev/null
sha256sum *.img *.iso 2>/dev/null
~~~

## Publishing numbers

A benchmark entry should state the exact hardware, software revision, methodology, number of runs, and whether the reported value is a median, mean, or single observation.
