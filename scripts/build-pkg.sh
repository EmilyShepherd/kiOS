#!/bin/bash
set -e

BUILT_PACKAGES=$(pwd)/.build/built
BUILD_DIR=$(pwd)/.build/root
INITRD=$(pwd)/.build/initrd
BOOTPART=$(pwd)/.build/bootpart
TARGET=aarch64-linux-musl

if test "$1" == "--clean"
then
  CLEAN=1
  shift
fi

pkg=$1
pkgpath=pkgs/$pkg

. $pkgpath/build

for dep in $depends
do
  test -f $BUILT_PACKAGES/$dep || $0 $dep
done

cd $pkgpath

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
  *.xz)
    extract=J ;;
esac

if test -n "$CLEAN"
then
  rm -rf src
fi

mkdir -p $BUILD_DIR $BUILT_PACKAGES ${INITRD}/{bin,lib} ${BOOTPART} src

if test -n "$url"
then
  if ! test -f srcpkg || ! test $(sha256sum srcpkg | head -c 64) == "$checksum"
  then
    rm -rf src
    curl -L $url -o srcpkg
  fi

  mkdir -p src
  tar -x$extract -C src --strip-components 1 -f srcpkg
fi

cd src

export GOARCH=arm64
export CGO_ENABLED=1
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-musl-
export PREFIX=""
export DESTDIR=$BUILD_DIR
export PKG_CONFIG_SYSTEM_LIBRARY_PATH=${BUILD_DIR}/lib
export PKG_CONFIG_LIBDIR=${BUILD_DIR}/lib
export PKG_CONFIG_PATH=${BUILD_DIR}/lib/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=${BUILD_DIR}
export CC=$(which ${TARGET}-gcc)
export CXX=$(which ${TARGET}-g++)
export STRIP=$(which ${TARGET}-strip)
export CFLAGS="-isystem ${BUILD_DIR}/include"
export LDFLAGS="-w -s"

if test "$(type -t pre_build)" == "function"
then
  pre_build
fi

case $type in
  configure)
    ./configure --prefix="" --host=aarch64-unknown-linux-gnu --with-sysroot=${BUILD_DIR} $configure_flags
    make $make_target
    make ${install_target:-install}
    ;;
  make)
    make $make_target
    make ${install_target:-install}
    ;;
  *)
    build
    ;;
esac

cd ..

if test -d files
then
  cp -r files/* ${BUILD_DIR}/
  cp -r files/* ${INITRD}/
fi

for file in $files
do
  cp -r ${BUILD_DIR}/$file ${INITRD}/$file
done

process_elf() {
  for lib in $(readelf -d ${BUILD_DIR}/$1 | grep NEEDED | grep -o '\[.*\]' | tr '[]' ' ')
  do
    if ! test -f ${INITRD}/lib/$lib
    then
      cp -L ${BUILD_DIR}/lib/$lib ${INITRD}/lib/
      process_elf lib/$lib
    fi
  done
}

for bin in $binaries
do
  path=${BUILD_DIR}/$bin
  if test -e $path
  then
    cp -d $path ${INITRD}/bin/
    process_elf $bin

    if test -L $path
    then
      cp -L $(dirname $path)/$(readlink $path) ${INITRD}/bin/
    fi
  fi
done

touch $BUILT_PACKAGES/$pkg
