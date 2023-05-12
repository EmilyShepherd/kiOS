# Data Partition

kiOS' root remains initramfs at runtime, so it does not use a
conventional "root" partition. Instead, permanent / disk-backed storage
is written to an ext4 partition referred to as the `datapart`.

## Structure

The data partition has the following structure:

- /meta/etc - System configuration files
- /meta/log - System and container logs
- /data/oci - OCI Layer, Image and Container storage
- /data/pods - Kubelet pod storage (eg emptydirs and other storage)
- /modules - Optional runtime loadable kernel modules.

At runtime, these directories are mounted to the locations they would
normally be expected:

- /meta/etc -> /etc (overlay mount, see below for details)
- /meta/log -> /var/log
- /data/oci -> /var/lib/containers/storage
- /data/pods -> /var/lib/kubelet/pods
- /modules -> /lib/modules

### Etc Directory

kiOS requires a few files to exist in `/etc`, so a few sensible default
options are bundled as part of the root initramfs:

- /etc/containers/policy.json - permissive image pulling policy
- /etc/containers/registries.conf - example registries.conf file
- /etc/crio/crio.conf - example crio.conf file
- /etc/resolv.conf - DNS Resolver Config (defaulting to CloudFlare 1.1.1.1)
- /etc/hosts - Static hosts file
- /etc/ssl/ca-bundle.pem - Latest Mozilla CA Bundle

kiOS also supports the following locations, although no defaults are
provided:

- /etc/kubernetes/kubelet.conf - Kubeconfig file for the kubelet to talk to the api-server
- /etc/kubernetes/bootstrap-kubelet.conf - Bootstrap Kubeconfig file for the kubelet to bootstrap itself
with the api-server
- /etc/kubernetes/credential-providers.yaml - CredentialProviderConfig file
- /etc/kubernetes/manifests - Directory for static pod manifests
- /etc/cni/net.d - Directory for CNI (Container Networking) configuration
- /etc/hostname - The name of the system

#### Containers Policy File

`/etc/containers/policy.json`

This file specifies container signature verification - on kiOS the
default is to accept unsigned certificates:

```json
{
    "default": [
        {
            "type": "insecureAcceptAnything"
        }
    ]
}
```

For more information about this file, see containers-policy.json(5).

#### Containers Registries File

`/etc/containers/registries.conf`

This file specifies registry / mirror settings. On kiOS the default is
to add a rule to use `docker.io` for unqualified images:

```toml
unqualified-search-registries = ["docker.io"]
```

This will result in unqualified images (eg `alpine:latest`) being
resolved to `docker.io/alpine:latest` when pulling.

NB: It is considered insecure to use unqualified images - where possible
it is advisable to always qualify your image names with full domain
name.

For more information about this file, see containers-registries.conf(5).

#### Crio Configuration File

`/etc/crio/crio.conf`

This file specifies settings specific to the container runtime - cri-o.
kiOS does not specify any unusual / specific settings in this file.

NB: The container runtime is core to the way kiOS works, so there are
some settings that will break the system if changed. As a result, the
following settings are specified by kiOS to the container runtime via
flags, which means they cannot be updated via this configuration file:

- default-runtime
- no-pivot
- cgroup-manager
- listen (local socket path)
- root (container storage)
- runroot (container runtime storage)
- conmon
- conmon-cgroup
- pinn-path

For more information about this file, see crio.conf(5).

#### Resolv Configuration

`/etc/resolv.conf`

This file is used to determine the dns resolvers that the container
runtime will use for pulling images, and that kubelet will use for
resolving the api server hostname.

The nameservers in this file will also be provided to pods who are using
"Default" dns settings (either because they are in the host network or
have explictly have the "Default" dnsPolicy).

The default values in kiOS core are CloudFlare's DNS servers:

```
nameserver 2606:4700:4700::1111
nameserver 2606:4700:4700::10001
nameserver 1.1.1.1
```

For more information about this file, see resolv.conf(5).

#### Hosts File

`/etc/hosts`

This file contains a static lookup table of hostnames to IP addresses.
The default value in kiOS is simply to specify the rules for localhost:

```
127.0.0.1   localhost.localdomain   localhost
::1         localhost.localdomain   localhost
```

NB: It is unusual that this file will need to be modified, as it is
normally better practice to configure hostname rules at a central
resolver level, but the option is available if required. 

Warning: If you modify this file, you should not delete the rules for
localhost.

For more information about this file, see hosts(5).

#### SSL Bundle File

`/etc/ssl/ca-bundle.pem`

This file contains a list of public x509 certificates to be trusted in
TLS connections. This is used by the container runtime when pulling
images over TLS.

This file is generated from the CA certificates bundled into Mozilla and
is generally accepted to be a good list of the "standard" trusted root
CAs.

#### Kubelet and Bootstrap Kubelet KubeConfig Files

`/etc/kubernetes/kubelet.conf` `/etc/kubernetes/bootstrap-kubelet.conf`

In order for the kubelet to talk to the cluster api-server, it needs to
know its address, and how to authenticate with it - this information is
typically stored in the `kubelet.conf` file. If the kubelet needs to
use a different authentication method for its initial bootstraping, it
will use the `bootstrap-kubelet.conf` file.

For more information about kubelet bootstrapping, see the [Kubernetes
Documentation](https://kubernetes.io/docs/reference/access-authn-authz/kubelet-tls-bootstrapping/).

#### Credential Provider Config File

`/etc/kubernetes/credential-providers.yaml`

In order for the container runtime to know how authenticate itself with
non-public container registries, kubelet needs to provide it with
relative auth credentials. It is sometimes desirable to configure these
at the node level, in which case they can be via this file.

For more information about the Credential Provider Configuration, see
the [Kubernetes Documentation](https://kubernetes.io/docs/tasks/administer-cluster/kubelet-credential-provider/).

#### Manifest Directory

`/etc/kubernetes/manifests`

When the kubelet starts up, it needs to know what to run. Normally, it
talks to the api-server to obtain the details of any pods it should be
running, however there are some Pods that kubelet needs to run _before_
it is able to talk to the api-server. Some examples may be:

- A DHCP Client (to obtain an IP address required to talk to the api-server)
- A Network Time Daemon
- An automatic disk partition resizer
- A bootstrap to configure the system at first boot

For example, [kios-aws](https://github.com/EmilyShepherd/kios-aws) specifies
a DHCP Static Pod, a disk resizer (as the actual volume size at runtime
often differs from the size of the original image), and an aws-specific
"aws-bootstrap" container, which is responsible for reading instance
metadata and bootstrapping the node.

#### CNI Configuration

`/etc/cni/net.d`

This folder is scanned for the desired configuration for pod networks.
This is often set at runtime by a DaemonSet Pod, however it can also be
configured statically.

For more information about container networking, see the [Kubernetes
Documentation](https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/network-plugins/).

#### Hostname file

`/etc/hostname`

If this file is present, kiOS will automatically update the kernel with
the given hostname at startup.
