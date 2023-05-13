# Installation

## Boot Partition

kiOS is a single EFI binary, so it should be placed in a GPT EFI Boot
vfat partition.

If it is the default / only OS to be run on the machine, it is often a
good idea to place it in the EFI's default path:

```
EFI/Boot/Bootx64.efi
```

## Data Partition

kiOS requires a `datapart` for disk-backed storage. For general
information about this partition, see [its documentation](./datapart.md).

For installation, ensure you have created an ext4 partition. Create a
directory at `/meta/etc/kubernetes/manifests` and place the Pod
manifests of any containers you'd like kiOS to run in it.

### Container Priming

In order to run containers that are not present on the local machine,
kiOS will first pull them down. This can sometimes create a chicken and
egg if one of the early containers is, say a dhcp daemon, as this needs
to have run before any images can be pulled.

To avoid this issue, kiOS supports container "priming" - the easiest way
to do this is with the following command:

```
> podman --root /path/to/datapart/data/oci pull [images...]
```

NB: If you are running in a network with IPv6 SLAAC, pre-priming is not
required, as a network presence can be established by the kernel without
much userspace intervention (kiOS will automatically bring `eth0` up at
early boot). However, for other network configurations pre-priming is
often required. If you are after zippy boot times, it can be a good
practice to do anyway.

If you are pre-priming, you **must** always include
`registry.k8s.io/pause:3.6` in the list of images you pre-prime.

## Kernel Parameters

By default kiOS will search for the `datapart` at `/dev/nvme0n1p2`. If
is located somewhere else, you should specify this via the kernel
command line `datapart` parameter:

```
datapart=/dev/nvme1n1p3
```

Currently, datapart does not support UUIDs or other block device
identifiers.
