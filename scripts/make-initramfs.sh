#!/bin/bash
#

if test "$1" == "--system"
then
  shift
  ./scripts/build-pkg.sh lib/musl
  ./scripts/build-pkg.sh cacerts

  for bin in busybox conmon cri-o crun init iptables kubernetes
  do
    ./scripts/build-pkg.sh bin/$bin
  done
fi

(
  cd .build/initrd
  find . | cpio -oH newc -R +0:+0 > ../initramfs.cpio
)

if test "$1" == compressed
then
  cat .build/initramfs.cpio | zstd -T16 -22 --ultra > .build/initramfs.cpio.zstd
fi

