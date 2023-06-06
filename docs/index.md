---
title: Home
---

# kiOS - The Kubernetes Operating System

<div style="text-align: center;" markdown>
  <img width=200 src="logo.svg" alt="kiOS" />

  kiOS is a tiny operating system, distributed as a single EFI binary,
  tailored to running Kubernetes nodes with minimal overhead and fast boot
  times.

  [Install kiOS :octicons-arrow-right-16:](setup/quick-start){ .md-button .md-button--primary }
</div>

## Why kiOS?

### kiOS is fast

kiOS doesn't have a large, complex or configurable early init system -
no initrc, no runlevels, no service files. As soon as the userspace is
ready, kiOS does a small amount of kubernetes-specific init, then
immediately launches the container runtime and kubelet with minimal
blocking.

### kiOS is secure

The host operating system doesn't have a shell at all (no bash, no vim,
not even utility commands like echo, cat, ls etc). This reduces the
attack vector as malicious scripts will not work on the host system - to
attack kiOS, you'd need to get a binary into the system.

The host's root filesystem is ephemeral - it only lives in RAM and most
mounts are restrictive, making injecting malicious binaries at runtime
much harder for would-be attackers.

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

!!! success "So, if you know how to create kubernetes pods, you know how to configure kiOS!"
