# Hardcore Linux

**A lightweight, independent Linux rootfs and distribution.**

Hardcore Linux is not a derivative of Arch, Debian, or any other distro — it is built from scratch, aiming to stay minimal without giving up functionality.

---

## Table of contents

- [Why Hardcore?](#why-hardcore)
- [Installation](#installation)
- [What's in the box](#whats-in-the-box)
- [Init system](#init-system)
- [Package management](#package-management)
- [Desktop environment](#desktop-environment)
- [Reporting bugs](#reporting-bugs)
- [License](#license)

---

## Why Hardcore?

Most Linux distros today ship a 2+ GB ISO — Ubuntu's full GUI variant weighs in at around 6 GB, and Omarchy doesn't even attempt to be minimal.

Some distros try harder:

| Distro | Base size | Trade-off |
|---|---|---|
| Arch | ~700 MB | Reasonable, but not truly minimal |
| Alpine | Smaller | Musl-based, but limited package selection |
| TinyCore | ~17 MB | Impressively tiny, but almost nothing works out of the box |

**Hardcore Linux** aims to sit in the sweet spot: minimal footprint, maximal usefulness.

Key design choices:

- **musl libc instead of glibc** — lighter, safer, and strictly POSIX-compliant.
- **Shell-script init system** — the init launches scripts and services from `/system`. A shell script is far easier to audit than an opaque binary.
- **No bloat by default** — you only get what you ask for.

In about **700 MB** you can fit the base Hardcore system *plus* GCC, G++, LLVM, Mesa, Python, the full Wayland dependency stack, a compositor, and a terminal emulator with a set of terminal apps — all while idling at a maximum of **~110 MB** of RAM (often as low as 20–30 MB; unused RAM is wasted RAM).

The official repository ships most of the software you'd need to build things yourself. For packages outside its scope (e.g. Rust), grab the upstream static build directly:

```
https://static.rust-lang.org/dist/rust-1.97.1-x86_64-unknown-linux-musl.tar.xz
```

---

## Installation

See the [Wiki](https://github.com/AMAZING2545/HardcoreLinux/wiki) for full installation instructions, or download the prebuilt `.img` directly if you'd rather skip the reading.

---

## What's in the box

The repository is organized as follows:

| Path | Contents |
|---|---|
| `build/` | Build recipes for the core system (musl, busybox, kernel, binutils, etc.) |
| `build.native/` | Build recipes for native/optional packages (Wayland stack, Sway, dev toolchains) |
| `configs/` | Kernel and Busybox `.config` files |
| `repo/` | Prebuilt package archives (`.tar`) served from the official repository |
| `tools/` | Core system utilities: init, package manager, sudo replacement, filesystem tools |

---

## Init system

Hardcore's init is a plain POSIX shell script (`tools/init`). On boot it:

1. Mounts `/proc`, `/sys`, `/dev/pts`, and tmpfs for `/run` and `/tmp`.
2. Starts `mdev` for hotplug device management.
3. Loads core drivers listed in `/system/drivers/modprobe`.
4. Spawns `getty` on multiple TTYs.
5. Runs every script in `/system/scripts`.

Services are managed at runtime with `initctl` (`start` / `stop` / `info`), which tracks running services under `/system/tmp/services`.

Being a readable shell script rather than a compiled binary, the entire boot process can be audited line by line.

---

## Package management

Hardcore currently ships **two** package managers, at different stages of maturity:

### `flashman` — the current default

A minimal POSIX shell script (`tools/flashman`) that installs packages by extracting a `.tar` archive onto `/` and running the package's own `install`/`uninstall` scripts from `/etc/installer/<package>/`. It performs a basic dependency check before installing.

```sh
flashman install <package.tar>
flashman uninstall <package>
flashman info <package>
flashman list
```

Simple, transparent, and easy to audit — but with no real dependency resolution, no upgrade path, and no conflict handling.

### `yspm` — the upcoming native package manager

Hardcore is actively integrating [`yspm`](https://github.com/Yassine-Jemi01/yspm), a system package manager written in Go, as a more capable successor to `flashman`. It brings:

- Proper dependency resolution and shared-library metadata
- A native `.yspkg` package format
- Lifecycle hooks and service integration
- Repository generation/signing and safe transactional installs
- A `yspm sync` command to import existing Hardcore/`flashman` packages

```sh
sudo yspm install gcc firefox
sudo yspm remove gcc
sudo yspm sync   # import legacy Hardcore packages
```

> **Status:** `yspm` is still under active development (currently v0.2.0) and is **not yet a drop-in replacement** for `flashman`. Expect both tools to coexist until the migration is complete.

---

## Desktop environment

Short answer: **yes, but it's opt-in and manual.**

The base system ships with no Wayland compositor at all. To get a graphical session:

1. Install the `sway` package. This will pull in a fairly large set of dependencies (Wayland core libraries, Mesa, etc.).
2. Install `udev` — it's required for `libinput` to detect your devices. After installing it, you'll need to write a small script to launch `udevd` and run `udevadm trigger`.
3. Set your renderer to **pixman**, not **opengl**. `wlroots` does not get along with `llvmpipe` (software OpenGL) and will crash on startup if you leave the default renderer set.

None of this is automated yet — treat it as a manual, "assemble it yourself" setup rather than a turnkey desktop.

---

## Reporting bugs

Found a bug or have a feature request? Please [open an issue](https://github.com/AMAZING2545/HardcoreLinux/issues).

---
## License

Hardcore Linux is licensed under **GPL-2.0** — see [`LICENSE`](LICENSE) for details.
