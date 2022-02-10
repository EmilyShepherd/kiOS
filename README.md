kiOS
====

A slim operating system tailored to run Kubernetes nodes with minimal
overhead and fast boot times.

## Core Features

kiOS is a distribution of Linux, designed to run (and be run by)
Kubernetes. It has the following design features:

- All system services, including cluster init, are run as static
  kubernetes pods - kubelet is used in place of traditional init systems
  like openrc or runsv to monitor / restart these services.
- The system attempts to be relatively unopinionated; system pod
  manifests the standard location and can be configured during runtime
  or on startup, in the same way you would configure any other k8s pod.
- kiOS is designed to bootstrap itself as a node or master by reading a
  single "init" file from the boot partition - interactive configuration
  is not required (or, indeed, supported!).
- Under the hood, containers are run with [cri-o][crio] and
  [crun][crun].
- The system treats its boot partition as read only by default, so kiOS
  can be run on a system with a slow or read only (eg flash) boot
  device, using another drive for data storage.

## Raspberry Pi 4

kiOS is currently only tailored for running on the raspberry pi 4 (in 64
bit mode), however further flavours may be added at some point.

By default, kiOS bridges the host network with all containers, giving
them their own L2 access to the physical network. DHCP is used to
obtain IP addresses for both the host and any containers.

Chrony is deployed as an NTP client, and is setup to access the generic
ntp server pool.

[crio]: https://github.com/cri-o/cri-o
[crun]: https://github.com/containers/crun
