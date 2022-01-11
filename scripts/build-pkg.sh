#!/bin/bash

BUILD_DIR=$(pwd)/build
DIST_DIR=$(pwd)/dist
TARGET=aarch64-linux-musl

if test "$1" == "--clean"
then
  CLEAN=1
  shift
fi

pkg=$1

cd pkgs/$pkg

. ./build

case $type in
  netfilter)
    url=http://www.netfilter.org/projects/${pkg}/files/${pkg}-${version}.tar.bz2
    type=configure
    ;;
esac

case $url in
  github)
    url=https://github.com/${project}/${pkg}/archive/refs/tags/${version}.tar.gz
    ;;
esac

case $url in
  *.bz2)
    extract=j ;;
  *.gz)
    extract=z ;;
esac

if test -n "$CLEAN"
then
  rm -rf src
fi

mkdir -p src $BUILD_DIR

curl -L $url | tar -x$extract -C src --strip-components 1

cd src

export GOARCH=arm64
export CROSS_COMPILE=aarch64
export PREFIX=$BUILD_DIR

case $type in
  configure)
    export PKG_CONFIG_PATH=${BUILD_DIR}/lib/pkgconfig 
    export CC=$(which ${TARGET}-gcc)
    ./configure --prefix=${BUILD_DIR} --host=${TARGET} $configure_flags
    make
    make install
    ;;
  make)
    make
    make ${install_target:-install}
    ;;
esac
