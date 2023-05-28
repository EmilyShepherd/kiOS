# Files

One of the core tenants of the kiOS project is to give cluster
administrators visibility and control over exactly what runs on their
systems. To that end, this document lists every file on the kiOS system,
and explains why they are present.

## Init

`/init`

The init file is the first process of the system - it is called directly
by the linux kernel after boot up. kiOS' init is a custom written
program, which performs the following functions:

- Setting up the base filesystem (mounting `/dev`, `/proc`, `/sys`, etc)
- Finding and mounting the data partition
- Starting the container runtime
- Starting the kubelet
- Starting and listening for commands via the system socket
- Monitoring all processes for shutdowns and:
    - Reaping zombie processes
    - Restarting the container runtime or kubelet if they crash

## Binaries

`/bin`, `/sbin`

### busybox

`/bin/busybox`,
`/bin/modprobe`,
`/bin/mount`,
`/bin/umount`,
`/bin/nsenter`

There are four linux utilities that kiOS needs to function - busybox
provides a single binary to perform all four of these:

- modprobe - used by the kernel for automatic module loading
- mount - used by kubelet for mounting some volumes
- umount - used by kubelet for unmounting some volumes
- nsenter - used by cri-o for entering some namespaces

### conmon

`/bin/conmon`

The Container Monitor (`conmon`) is the process which sits between the
container runtime and the container itself. It is responsible for:

- Holding open the container's stdin (so that exec'ing into the
  container on demand can happen)
- Sending the container's stdout and stderr to disk
- Recording the exit status and time, when the container terminates

### crio

`/bin/crio`

Cri-o is the container runtime. It is a long running process,
responsible for taking instruction from the kubelet, pulling and
verifying container images, setting up pod environments (eg port
forwarding, container networking etc), and starting containers (by
calling [crun](#crun))

### crun

`/bin/crun`

Crun is the low level OCI runtime. It is responsible for running a
container. It is ultimately the process that will setup the container's
root, namespaces, linux capabilities, and seccomp rules.

### iptables

`/bin/xtables-legacy-multi`,
`/bin/iptables`,
`/bin/iptables6`,
`/bin/iptables-restore`,
`/bin/iptables6-restore`

IPTables is the userland program which setups rules in linux's iptables
firewall / filtering system. It is used by cri-o to implement port
forwarding and kubelet for some legacy rules.

### kubelet

`/bin/kubelet`

Kubelet is the component of kubernetes which runs directly on each node.
It is responsible for managing what should be run, and in what
configuration. If the node is running as part of a cluster, it normally
takes its lead from the cluster's api-server, however it is also able to
manage "static pods" based on local configuration.

For kubernetes-specific configuration (for example volume plugins,
config maps, etc), kubelet manages these itself. For more generalised
tasks, such as pulling images, creating pod environments and running
containers, it instructs cri-o.

### pinns

`/bin/pinns`

Pinns is a small stand alone executable which is used by cri-o to setup
namespace and sysctl items for containers.

## Runtime Libraries

`/lib`, `/lib64`

### Lib C

`/lib/libc.so`,
`/lib/ld-linux-x86-64.so.2`,
`/lib/ld-musl-x86_64.so.1`

The C Library contains the core basic utilities that all binaries
require. This includes, for example, an understanding of reading /
writing files, allocating memory, creating threads, and performing DNS
lookups.

There are several implementations of Lib C - the most common is the GNU
C Library (glibc), however on kiOS we use the musl implementation, which
is more lightweight, but completely acceptable for kiOS' needs.

#### ld-musl-x86_64.so.1

Musl's libc also does the job of the dynamic linker - which is used to
pull runtime libraries when a program executes. All of the binaries on
kiOS' system expect this to be called `ld-musl-x86_64.so.1` so the
symlink is present.

#### ld-linux-x86-64.so.2

Some CNIs, which are installed into the base system, incorrectly declare
themselves as dynamically linked (this is bad practice, ahem AWS)
against the GNU dynamic linker: `ld-linux-x86-64.so.2`. As a result a
symlink is provided for this too.

### Glib

`/lib/libglib-2.0.so.`

Glib is a library written by GNOME for some generic low level / memory
management related tasks.

This library is only used by [conmon](#conmon).

### PCRE

!!! note
    This library is not strictly required by the system, and we hope to
    remove it shortly

`/lib/libpcre2-8.so`

The Perl-Compatible Regular Expressions library is required by glib.

