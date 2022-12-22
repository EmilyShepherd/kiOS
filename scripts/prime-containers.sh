#!/bin/bash

images=(
  registry.k8s.io/pause:3.6
  docker.io/emilyls/dhcp:9.4.0
  docker.io/emilyls/dhcp-cni:v1
  docker.io/emilyls/chrony:4.2
  docker.io/emilyls/bootstrap:v1
)

podman --root .build/datapart/containers/storage pull --platform=linux/arm64 "${images[@]}"
