# Kubelet

The kubernetes host client - manages the lifecycle of kubernetes pods on
this node.

## Build notes

We build kubelet _without_ any legacy docker support - this is not
needed as we use cri-o instead.

We also build kubelet without specific cloud / provider support - this
is the direction kubernetes is heading anyway, but it means you MUST use
provider specific plugin deployments with kiOS (for example, for EBS /
EFS etc) if these are required.

## Shutdown Behaviour

Kubelet includes a shutdown hook to cleanly shutdown and evict pods when
a system shutdown is detected. Upstream, this only supports the systemd
/ dbus shutdown ecosystem, which is not present on kiOS. We build a
patched kubelet, with a modified shutdown handler for kiOS.
