# Cri-o

The Kubernetes Container Runtime Interface - this is responsible for
pulling images / creating networking etc under the direction of kubelet.

## Support

Cri-o currently has support for seccomp profiles, however no support for
verifying gpg image signatures. This is coming soon.

## pinns

This build also creates the `pinns` binary, which is responsible for
setting up namespaces on behalf of cri-o.
