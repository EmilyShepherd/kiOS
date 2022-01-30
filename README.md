KiOS
====

A slim operating system tailored to run Kubernetes nodes with minimal
overhead and fast boot times.




## Installation

*Requires `aarch64-linux-musl-*` cross compilers*

1. Build the boot partition:

```
./scripts/build-pkg.sh bootpart
```

2. Format an SD with two partitions, the first ~250M vfat, the second
can be the remainder of the drive and should be ext2, ext3, or ext4.

3. Copy .build/bootpart/* into the first partition. **Do not change the
file structure**. The second partition can be left empty.

4. You can now boot the pi from the SD card. Currently, once the system
is booted once, you need to turn it off, and pull the admin kubeconfig
from it in order to access the cluster. It will be in the first
partition, under `etc/kubernetes/admin.conf`. This process will be
improved soon.
