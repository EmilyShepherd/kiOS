kiOS
====

A Small Operating System designed to run Kubernetes on a Raspberry Pi 4.


## Installation

*Requires `aarch64-linux-musl-*` cross compilers*

1. Build the base system:

```
./scripts/build-pkg.sh system
```

2. Build the boot files:

```
./scripts/build-pkg.sh bootpart
```

3. Format an SD with two partitions, the first ~250M vfat, the second
can be the remainder of the drive and should be ext2, ext3, or ext4.

4. Copy .build/bootpart/* into the first partition. **Do not change the
file structure**. The second partition can be left empty.

5. You can now boot the pi from the SD card. Currently, once the system
is booted once, you need to turn it off, and pull the admin kubeconfig
from it in order to access the cluster. It will be in the first
partition, under `etc/kubernetes/admin.conf`. This process will be
improved soon.
