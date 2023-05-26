# Version Support

kiOS is currently in alpha - its only release branch is based on
Kubernetes v1.25.

!!! info "This means that kiOS nodes all run kubelet v1.25.x"

## Why not a more up to date kubelet?

During its alpha phase, it does not make sense to maintain separate
release versions for different versions of kubernetes. This is because
the focus at this time should be on fixing bugs / responding to
feedback - the ultimate aim being to move towards a "stable" version.

Kubernetes' [Version Scew Policy][k8s-versioning] states that the
api-server will always maintain backwards compatibility with kubelets
two versions behind. Ie, a kube-apiserver at v1.27 will support kubelets
of v1.27, v1.26, v1.25.

As a result of this policy, kiOS will remain pegged at _2 releases
behind_ the latest version of Kubernetes. This gives kiOS the biggest
compatibility for testing on clusters, as kiOS' single release branch
can be tested on clusters of v1.25, v1.26, and v1.27.

[k8s-versioning]: https://kubernetes.io/releases/version-skew-policy/

## Updates During Alpha

When Kubernetes v1.28 releases, kiOS will upgrade its kubelet to 1.26.y,
to maintain the 2 version scew.

While the `v1.25.0-alphax` releases will remain available, 1.25 will not
receive any further support at that point.

## Road to Stable

If and when kiOS becomes stable, release versions will be maintained for
all in-life Kubernetes versions going forward (for the avoidance of
doubt: at the time of writing Kubernetes 1.24 is still in life, however
kiOS will not back support and create a stable release for this version).
