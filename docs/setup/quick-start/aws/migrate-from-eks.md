# EKS Migration Guide

!!! note
    This guide is for users with an existing "vanilla" EKS cluster. If
    you are setting up a cluster from scratch, follow [this guide](new-cluster.md)
    instead.

## AWS Permissions

kiOS nodes use the same bootstraping logic as vanilla EKS images, so
your existing EC2 Instance Profiles should be sufficient for kiOS.

kiOS nodes **do not** require the `AmazonEKSWorkerNodePolicy` policy, so
you can omit this from Instance Profiles.

### Pulling Images

As with "vanilla" EKS nodes, if you are planning on pulling AWS ECR
Images, nodes should have the appropriate permission to pull them. Often
this is achieved via the following policy:

```
arn:aws:iam::aws:policy/AmazonEC2ContainerRegistryReadOnly
```

## Instance User Data

Vanilla EKS nodes treat their instance user data as an executable
script, and as a result, you may be used to setting your userdata to
look something like this:

```bash
#!/bin/bash
set -e

B64_CLUSTER_CA="Y2VydGlmaWNhdGUK"
API_SERVER_URL="https://xxxxxxxxxxxx.gr1.eu-central-1.eks.amazonaws.com"

/etc/eks/bootstrap.sh cluster-name --b64-cluster-ca $B64_CLUSTER_CA --apiserver-endpoint $API_SERVER_URL
```

kiOS does **not** treat instance metadata user data as an arbitrary
executable. Instead, it is parsed as a YAML configuration file, in the
following format:

```yaml
apiVersion: kios.redcoat.dev/v1alpha1
kind: MetadataInformation
apiServer:
  name: EKS-CLUSTER-NAME
  endpoint: EKS-CLUSTER-URL
  b64ClusterCA: BASE64-EKS-CLUSTER-CA-CERTIFICATE
```
