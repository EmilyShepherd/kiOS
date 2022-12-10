kiOS
====

kiOS - the Kubernetes Operating System - is tailored to run Kubernetes
nodes with minimal overhead and fast boot times.

## Core Features

kiOS does not include a conventional init system - instead it is
designed to promptly launch the container runtime and then hand over
control to kubernetes.

As a result of this most system services (dhcpc, ntpc etc) run as Pods
and are configured via standard kubernetes manifests.

## Why kiOS?

### kiOS is fast

kiOS doesn't have a large, complex or configurable early init system -
no initrc, no runlevels, no service files. As soon as the userspace is
ready, kiOS does a small amount of kubernetes-specific init, then
immediately launch the container runtime and kubelet with minimal
blocking.

On a Raspberry Pi 4 (in 64 bit mode), this allows kiOS to have the first
containers running on the system within the first 12 seconds of kernel
time.

### kiOS is secure

The host operating system doesn't have a shell at all (no bash, no vim,
not even utility commands like echo, cat, ls etc). This reduces the
attack vector as malicious scripts will not work on the host system - to
attack kiOS, you'd need to get a binary into the system.

The host's root filesystem is ephemeral - it only lives in RAM and is
mounted as readonly, making injecting malicious binaries at runtime much
harder for would-be attackers. kiOS' boot partition, which is just used
at runtime for kernel modules, is also mounted as read only.

Finally, kiOS' simplicity makes hiding malicious binaries much
harder, even if you were able to circumvent the protections above. kiOS
only has a `/bin` directory (no `/usr/bin` or `/sbin` etc) and this
contains just 7 binaries. At runtime, there are only three long term
processes in the host pid namespace - init itself, the container
container runtime, and kubelet). kiOS' lack of complexity makes hiding
an 8th binary, or a 4th running process almost impossible.

### kiOS is configurable

kiOS runs as little as possible outside of kubernetes - which means most
system services run as kubernetes pods. This even includes services that
need to exist before networking is ready - for example the ntp (time) or
dhcp clients - which run as static pods.

The model of kubernetes running host level services is already well
established with cluster CNI / CSI plugins. Installing services like AWS
VPC CNI, or Calico, etc - run as pods, but have host level
ramifications.

**So, if you know how to configure kubernetes, you know how to configure
kiOS!**

## Raspberry Pi 4

kiOS is currently only tailored for running on the raspberry pi 4 (in 64
bit mode), however further flavours may be added at some point.

By default, kiOS bridges the host network with all containers, giving
them their own L2 access to the physical network. DHCP is used to
obtain IP addresses for both the host and any containers.

Chrony is deployed as an NTP client, and is setup to access the generic
ntp server pool.
