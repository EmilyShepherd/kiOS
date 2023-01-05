# Busybox

We use busybox as a single small build for a handlful of required core
utilities:

- modprobe
- nsenter
- mount
- umount

## Modprobe

Modprobe is used automatically by cri-o, kubernetes and iptables to auto
load required kernel modules, if they were not already built into the
kernel.

**NB:** We may drop support for modprobe and instead require that all
needed modules for core kiOS functionality are built into the kernel.

## NSEnter

The usage of the nsenter binary is not well documented, however it
appears as though it is used by cri-o as part of its CNI process.

## Mount / UMount

The usage of mount binaries are not well documented, however they appear
to be used in parts of the cri-o / kubernetes code bases.
