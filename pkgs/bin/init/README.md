# Init

kiOS' init process is custom built for the OS. It is designed to prepare
the system, and then launch the container runtime (Cri-o) and the
kubelet.

This init is intentionally not very configurable - it does not support
runlevels or optional run-at-startup services. It is hard coded to
follow the init process required to start kubelet. The expectation is
that all other system / host services should be run as (static) pods
to be locked down as containers and managed by kubelet.

The kiOS init is as follows:

- Mount the pseudo-filesystems:
  - `proc`
  - `sysfs`
  - `devfs`
  - `devpts`
  - `securityfs`
  - `tmpfs` on /run, /tmp, /var/cache, /var/tmp, and /var/run
  - /var/run is mounted as shared (to allow containers to access ns
  mounts in this folder
  - The cgroup v1 filesystems (kubernetes does not yet support cgroup v2
  so we do NOT mount v2 or "unified" cgroup setups)
- Mount the persistent data partition:
  - (datapart)/modules -> /lib/modules
  - (datapart)/log -> /var/log
  - (datapart)/lib -> /var/lib
  - (datapart)/etc/ -> /etc
- Auto sets the system hostname from /etc/hostname, if it is present
- Start crio
- Start kubelet
  - If a config file is found for kubelet, kubelet will be started with
  this passed to it
  - If not, kubelet will first be started in standalone mode so that it
  can start its static pods. If any service creates a kubelet config
  file, init will restart kubelet automatically.
  - NB: This process will only occur _once_. Subsequent changes to the
  config file will be ignored by init.
- From this point on, init will do very little, other than reaping any
orphaned zombie processes.

## Configuration

As stated above, there is very little supported configuration. However,
the data partition can be specified via the `datapart` environment
variable. This is set by the kernel automatically when adding
`datapart=xxx` to the kernel's command line.

If not specified, the default value is `/dev/sda`. NB: We do not
currently support UUIDs.
