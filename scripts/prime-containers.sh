#!/bin/bash
#

images=(
  registry.k8s.io/pause:3.6
  docker.io/emilyls/dhcp:9.4.1
  docker.io/alpine:3.17.3
  docker.io/emilyls/aws-bootstrap:v1.25.0-alpha4
  602401143452.dkr.ecr.eu-central-1.amazonaws.com/amazon-k8s-cni:v1.12.2-eksbuild.1
  602401143452.dkr.ecr.eu-central-1.amazonaws.com/amazon-k8s-cni-init:v1.12.2-eksbuild.1
  602401143452.dkr.ecr.eu-central-1.amazonaws.com/eks/kube-proxy:v1.25.6-minimal-eksbuild.1
)

podman --root .build/datapart/lib/containers/storage pull --platform=linux/amd64 "${images[@]}"

cd .build/datapart/lib/containers/storage

rm -rf overlay-containers libpod overlay-layers/*.gz defaultNetworkBackend *.lock */*.lock
