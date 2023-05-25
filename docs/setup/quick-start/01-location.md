# Installation Location

kiOS is installed in two parts:

- The EFI binary is installed to an EFI System Partition
- Everything else is installed to a kiOS-specific Data Partition

## Data Partition

!!! info
    The most important location in the data partition is the `etc`
    directory which is found in `datapart:/meta/etc` but will be mounted
    to the standard location `/etc` during runtime. During the next
    installation steps, references to saving files in `/etc/...` should
    be taken to mean `datapart:/meta/etc/...`

kiOS requires a data partition for disk-backed storage. For general
information about this partition, see [its documentation](../../technical/datapart.md).

If your disk is partitioned with GPT, it is recommended to set this
partition's type to the following GUID so that kiOS can auto discover
it:

```
97aac693-d920-465a-94fe-eb59fc86dfaa
```

??? question "What is this?"
    GPT Partitions have a "type" associated with them, which is
    identified via a globally unique identifier (GUID). kiOS's data
    partition's GUID is `97aac693-d920-465a-94fe-eb59fc86dfaa`.

This partition should have an ext4 file system created with it.


## Installing the EFI (Boot Partition)

kiOS is a single EFI binary, so it should be installed on a
EFI-compatible system. This means the binary should be placed on the EFI
System Partition.

If it is the default / only OS to be run on the machine, it is often a
good idea to place it in the EFI's default path: `EFI/Boot/Bootx64.efi`,
however it can be placed elsewhere and configured via your system's
UEFI Boot Order settings.

### Kernel Parameters

If you have setup the data partition with the GPT Type GUID specified
above, kiOS will auto-discover and mount the partition, however if you
have installed it in a non-standard location, you may specify it
explicitly via the kernel command line `datapart` parameter:

```
datapart=/dev/nvme1n1p3
```

Currently, datapart does not support UUIDs or other block device
identifiers.
