kiOS
====

kiOS - the Kubernetes Operating System - is tailored to run Kubernetes
nodes with minimal overhead and fast boot times.

## Core Features

kiOS does not include a conventional init system - instead it is
designed to promptly launch the container runtime and then hand over
control to the kubelet.

As a result of this most system services (dhcpc, ntpc etc) run as Pods
and are configured via standard kubernetes manifests.

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
runtime, and kubelet). kiOS' lack of complexity makes hiding an 8th
binary, or a 4th running process almost impossible.

### kiOS is configurable

kiOS runs as little as possible outside of kubernetes - which means most
system services run as kubernetes pods. This even includes services that
need to exist before networking is ready - for example the ntp (time) or
dhcp clients - which run as static pods.

**So, if you know how to create kubernetes pods, you know how to
configure kiOS!**

## Support

kiOS has build support for EFI boots on both amd64 (x86_64) and arm64
(aarch64) systems. aarch64 support has not been tested in a while.

A version of AWS is bundled [here](https://github.com/EmilyShepherd/kios-aws).

### Versioning

During its alpha stage, kiOS will not maintain separate release
versions for different versions of kubernetes. This is because the focus
at this time should be on fixing bugs / responding to feedback /
preparing for stable versions.

Instead, kiOS' kubelet will remain pegged at 2 releases behind the
latest version of Kubernetes, to match Kubernetes' backwards
compatibility version skew policy. This gives kiOS the biggest
compatibility for testing on clusters.

For example, if the latest release of Kubernetes is 1.27.x, the kiOS
kubelet will be 1.25.y. This means that kiOS will be compatible with
clusters running an api-server of version 1.27, 1.26, and 1.25.

When Kubernetes 1.28 releases, kiOS will upgrade its kubelet to 1.26.y.
While the 1.25.0-alphax releases will remain available, 1.25 will not
receive any further support at that point.

If and when kiOS becomes stable, release versions will be maintained for
all in-life Kubernetes versions going forward (for the avoidance of
doubt: at the time of writing Kubernetes 1.24 is still in life, however
kiOS will not back support and create a stable release for this version).
