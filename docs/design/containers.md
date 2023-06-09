# Everything Runs In Containers

Under kiOS, we run as much as possible inside of containers, with the
understandable exceptions of the container runtime and kubelet
themselves!

Running as much as possible is not a new concept in the containerised /
k8s world - for example, when setting up a Kubernetes cluster with
`kubeadm`, it is common practice to have the master nodes run the
control plane components (which are responsible for managing pods)
_inside_ static pods themselves. kiOS, however, extends this concept
further, by also running essential system daemons, and kubelet
configuration scripts, inside pods.

In a typical Kubernetes setup, we would often have a set of tasks to
carry out at startup:

1. Determine an IP address and hostname for the node (possibly via a
   DHCP client
2. Determine the details of the api-server to connect to (often via a
   bootstrap script which is specific to the environment the node is
   running in)
3. Start the kubelet, which will then connect to the api-server
   determined in step 2, using the IP address and node name determined
   in step 1.

Clearly, this suggests that steps 1 and 2 have to be performed _before_
kubelet can startup correctly. However - kiOS still runs these within
containers, within Kubernetes pods.

## How Does This Work In Practice

kiOS amends this setup - kubelet's default configuration is to run in
"standalone mode". In this mode, kubelet will not attempt to connect to
an api-server. Instead, the pods it will run are determined by
static pod manifests, which are written to disk.

These manifests specify all of the pods and containers which need to
run; in most setups these static containers will run a one-shot DHCP
client daemon to obtain an IPv4 address, and will run a cloud-specific
bootstrap script to determine the correct kubelet configuration.

When ready, the kubelet is then restarted with the new configuration,
which will cause it to then connect to the api-server.

## Reasoning

The reasoning behind running everything in containers fits into three
categories:

- Startup speed
- Security
- Configurability (and observability!)

### Startup Speed

The above example explains how kiOS boots up _in the case that it needs
to run a bootstrap script_. However there are a number of situations in
which no such script is required, for example:

- In the case that you have configured the node statically (eg in a bare
  metal / custom installation)
- In the case that the node has already been configured, and this is a
  restart

In both of these situations, the kubelet already has the information it
needs, at boot time, to connect.

By having an attitude of "getting to a stable kubelet _as soon as
possible_", we ensure that, in as many situations as possible, kubelet
is ready, and connected to the api-server, as soon as possible, which
can be critical for nodes when pods are in a pending state waiting to be
scheduled.

### Security

Most system daemons require some form of privilege, for example:

- DHCP requires network administration (to set and update ip addresses
  and routes), and network raw (to send non-IP packets for the DHCP /
  ARP protocols) capabilities.
- A module loader requires access to system modules, and the ability to
  request that the kernel load them.
- A utility for setting the system time requires the capability to do
  so.
- A utility to auto-resize a disk partition requires full and raw read
  access to the block device it is updating.

However, it is rare that any such daemon requires _all_ permissions that
running as root in the host namespace would grant them. For example,
none of the above services require:

- Write access to the host file system
- Visibility of running processes or other containers
- Permission to update file permissions, or bypass file permissions
- The ability to kill processes
- The ability to listen on system ports
- Permission to shutdown or reboot the system
- Read access to the system log
- Privileged access to the system virtual terminals

All of the above capabilities are implicitly granted by default under
Linux when running as root. While it is possible for a program to
voluntarily demote itself, not all do, and we already have a resilient
system to sandbox a process and grant only a small subset of
capabilities to it: containers!

### Configurability and Observability

One major drawback of running Kubernetes on a generalised operating
system is that it can be harder for a cluster administrator to have
visibility of _what is running on their servers_.

The method of viewing the system processes, or their logs, depends
entirely on the init system in use - which may vary from node to node,
and as a result, the host space is often ignored as a "black box" by
administrators.

The same issue applies when it comes to configuration - some systems
configure daemons via service files, others simply use scripts to call
the desired programs, others use single configuration files.

Under kiOS, visibility and configuration of system services is achieved
_exactly the same way_ as one would with any other service in a cluster,
because they _are the same as any other service in the cluster_.
