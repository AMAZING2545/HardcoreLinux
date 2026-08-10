# HARDCORE LINUX, a lighweight independent linux rootfs and distro

### installation: see wiki, or download the .img directly if you're impatient

### goals
most linux distros nowadays can start at 2+ GB ISO, the most notable example being Ubuntu with a 6GB ISO for the full GUI variant
or Omarchy (which does not even try to be minimal)

Some distros try, like Arch delivers a 700MB base rootfs; and some even harder, like Alpine, and some tried too hard, like  
TinyCore (impressive for 17MB, but almost nothing works)

Hardcore tries to be minimal while not giving away functionality.

Hardcore, unlike many distros, uses musl libc instead glibc, which is lighter, safer and strictly POSIX

the init system is a shell script that launches scripts and services from /system. a shell script is more auditable than a binary

the package manager is also a script

in 700MB you can fit the base Hardcore system, GCC, G++, LLVM, MESA, Python, many dependencies for Wayland, Wayland itself, a 
Compositor, A terminal emulator with many terminal apps

All of this with 110MB of idle memory usage at maximum (can go to 20/30MB, but free RAM is wasted RAM)

the repository only contains most software that you would need to compile your own(if you ever need rust, download it from there "https://static.rust-lang.org/dist/rust-1.97.1-x86_64-unknown-linux-musl.tar.xz")

### does Hardcore have a desktop environment?

yes and no

while the base system lacks wayland altogether, it can be installed via the sway package, which will prompt to install a lot
of dependencies

you will also need udev for libinput to work. after installing udev you will need to create a script and launch udevd and udevadm

then you need to set the renderer to pixman and not opengl(which will crash because wlroots does not like llvmpipe)

### report bugs
if you ever find a bug or request a feature, start a issue at https://github.com/AMAZING2545/HardcoreLinux/issues
