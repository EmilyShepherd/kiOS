# Building

The build is currently mostly designed for (and has therefore only been
tested against) cross compiling for aarch64 (arm64/v8) on an amd64 build
machine.

Support for other architectures will be looked into soon.

## Host Build Requirements

- aarch64-linux-musl-* compilers
- podman
- make
- meson / ninja
- cpio
- xz
- gzip
- go
- bison
- rsync
- cmake
- pkg-config
- gperf

## Build system

The system is split up into a series of "packages" - the design for
these was inspired by common build systems like xbps / apk / pacman etc.
Each "package" has its own build directory under pkgs, with a "template"
file teaching the build system what kind of build it is.

The build system will keep track of package dependencies, but it is
important to note that this is just for the purpose of an easy install:
the final OS is compacted down into an initramfs and does not support
adding or removing packages.

The main command to use is:

```
./scripts/build-pkg.sh [package name]
```

This will auto download the source files (if not already cached), unpack
them, build the project, and then copy any required binaries, files and
libraries into the initrd working file system.

By default, the build system will not clean out a directory when
building if it does not appear to need it, so that previous build
results can be cached by Make etc when possible. If you need to do a
clean build, pass `--clean`:

```
./scripts/build-pkg.sh --clean [package name]
```

The best packages to build is `initramfs`, which will build a complete
build and compress it into an initramfs. Building `bootpart`
additionally compiles the kernel image, the kernel modules, and
downloads and unpacks the Raspberry Pi 4 firmware.
