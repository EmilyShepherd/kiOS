# Static Pod Configuration

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

## Setup

By default, kubelet is configured to look in `/etc/kubernetes/manifests`
for any YAML files containing Pod Manifests. If these are found, kubelet
will start running these.

!!! warning
    Be aware that Static Pods cannot rely on the presence of a cluster.
    As a result, features such as ConfigMaps, Secrets, Services, etc are
    not supported.
