# Static Pod Configuration

The default location for static manifests is
`/etc/kubernetes/manifests`. If any YAML files are found in this
directory, kubelet will treat these as Pod manifests and run them
accordingly.

??? question "What are Static Pods?"
    Kubernetes supports running Pods on a node in one of two ways:

    - Via the api-server
    - Via static pods

    Pods running via the api-server is the process that most users will
    likely be most familiar with - this is when you deploy manifests to a
    cluster via kubectl, helm, etc, and then workloads are dynamically
    scheduled onto nodes.

    This is by far the preferred option, and should normally be used in most
    cases. However, kubelet supports running pods via "static" manifests -
    files stored directly on the node itself.

    There are a few use cases where this is useful:

    - If you are running kiOS as a standalone / embedded system, in which
    case you want it to run its workloads independently from any control
    plane.
    - Some services have to run _before_ the kubelet is able to communicate
    with a cluster. Examples of these might be a DHCP client daemon (to
    setup a network presence for the kubelet), or a "bootstrap" container,
    which determines node configuration and sets up the kubelet.

!!! warning
    Be aware that Static Pods cannot rely on the presence of a cluster.
    As a result, features such as ConfigMaps, Secrets, Services, etc are
    not supported.

!!! tip
    Do not forget that Static Pods are exactly that: _Pods_. Any YAML
    files in this directory should be `kind: Pod`. `DaemonSets`,
    `Deployments`, `StatefulSets` etc not supported.

kiOS is strictly unopinionated about what the pods you choose to run, so
the choice of how to configure your static pods is largely up to you. Be
aware that the kiOS base system does _not_ run any of the services you
may normally take for granted - depending on your use cases, you may
wish to run some / all / none of these as static (or api-server managed)
pods:

- a DHCP client (for obtaining IP addresses for the network)
- udev (for automatically loading kernel modules based on devices
  discovered / plugged into the system)
- sshd (for remote access to the system)
- agetty (for displaying an interactive shell to the user over serial
  port / using a physical screen)

### Node Pod

It is often desirable to perform small bits of setup / node specific
configuration / oneshot tasks on a node. It is sometimes useful to
collect these under a single pod named `node` so that node specific
logs can easily be accessed when synced to the api-server.

#### Node Level Log Mirroring

The logs for kiOS' kubelet and crio processes, which run outside of
containers at the host level, are attached to the following named pipes:

- `/var/run/crio.log`
- `/var/run/kubelet.log`

This means that the kernel will buffer their output until such time as
another process reads from these files. The expectation is that a
container will perform this role.

kiOS has provided a tiny container, who's job it is to do this, if this
behaviour is desirable: `docker.io/emilyls/tinycat`.

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: node
  namespace: kube-system
spec:
  containers:

  # Send crio logs to container stdout so that they can be viewed via
  # standard `kubectl logs node-xxx crio` command.
  - name: crio
    image: docker.io/emilyls/tinycat:v1-alpha2
    volumeMounts:
    - name: crio-log
      readOnly: true
      mountPath: /in

  # Send kubelet logs to container stdout so that they can be viewed via
  # standard `kubectl logs node-xxx kubelet` command.
  - name: kubelet
    image: docker.io/emilyls/tinycat:v1-alpha2
    volumeMounts:
    - name: kubelet-log
      readOnly: true
      mountPath: /in

  securityContext:
    readOnlyRootFilesystem: true
    capabilities:
      drop:
      - ALL

  volumes:
  - hostPath:
      path: /var/run/crio.log
    name: crio-log
  - hostPath:
      path: /var/run/kubelet.log
    name: kubelet-log
```

#### One shot tasks

There are a few one shot tasks that may be useful to setup a node -
these may be run as init containers within the node pod. An advantage of
init containers is that kubelet will run them in the order that they are
defined, so you ensure dependant tasks will wait until ready to run.


##### Obtain an IP address

```yaml
- name: dhcpcd
  image: docker.io/emilyls/dhcp:9.4.1
  args:
  # Normally DHCP undoes all of its settings when it terminates. We do
  # not want it to do so in this context, as its common for DHCP to be
  # restarted during the boot sequence when the system's hostname
  # changes. We do not want to interrupt our internet connectivity.
  - --oneshot

  # kiOS does not run udev, so thankfully we can guarantee that there
  # will be an ethernet adaptor called "eth0".
  - eth0
  volumeMounts:
  - mountPath: /var/run/dhcpcd
    name: runtime
    subPath: run
  - mountPath: /var/db/dhcpcd
    name: runtime
    subPath: db
  # Currently we require running as root but this may change
  securityContext:
    privileged: true
    capabilities:
      drop:
      - ALL
      add:
      # Required to allow DHCPCD to set IP addresses and routes on the
      # ethernet card, based on the response from the DHCP server.
      - NET_ADMIN

      # Required so that DHCPCD can send out DHCP messages which are
      # not one of the managed Linux Socket Types.
      - NET_RAW
```

##### Load Kernel Modules

```yaml
- name: modprobe
  image: docker.io/emilyls/modprobe:v1-alpha1
  args:
  - -a

  # Tiny Power Button is a small ACPI driver which automatically sends
  # signals to init when a hw pwr button is pressed which will trigger a
  # graceful node shut down.
  - tiny-power-button

  # kiOS has a strict policy of "not unless we know we'll need it"
  # policy. As a result, it _doesn't_ support shebangs out of the box.
  # This is fine as all of the kiOS early boot code / bootstrap
  # containers exclusively use compiled binaries. However this is
  # almost certainly going to be unexpected / undesirable to many users,
  # so we can enable shebangs explictly here.
  - binfmt-script

  volumeMounts:
  # As a wise man once said, "in order to be able to load kernel
  # modules, we must first be able to see the kernel modules"
  - mountPath: /lib/modules
    name: modules
    readOnly: true

  securityContext:
    readOnlyRootFilesystem: true
    capabilities:
      drop:
      - ALL
      add:
      # Required to allow modprobe to load the modules
      - SYS_MODULE
```

##### Resize Disk

```yaml
  # Parted Container
  #
  # Used to resize the data partition to fill up 100% of the space on
  # the physical disk. This is useful as the AMI snapshot is relatively
  # small <1GB however in almost all situations, we are likely to have a
  # larger disk than this. It is very common in cloud providers to do an
  # early stretch-to-size at boot time.
  - image: docker.io/emilyls/parted:v2
    name: parted
    securityContext:
      # Required to allow parted to open the block device
      privileged: true
    args:
    - "-sf"
    - /dev/nvme0n1
    - resizepart
    - "2"
    - "100%"

  # Resize2fs container
  #
  # Similar to the above, resize2fs' job is to expand the ext4 file
  # system to fill the newly resized data partition.
  - image: docker.io/emilyls/resize2fs:v1
    name: resize2fs
    securityContext:
      # Required to allow resize2fs to open the block device
      privileged: true
    args:
    - /dev/nvme0n1p2
```
