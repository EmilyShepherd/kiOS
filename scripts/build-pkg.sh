#!/bin/bash
set -e

PROJROOT=$(pwd)
BUILT_PACKAGES=$(pwd)/.build/built
BUILD_DIR=$(pwd)/.build/root
INITRD=$(pwd)/.build/initrd
BOOTPART=$(pwd)/.build/bootpart
TARGET=aarch64-linux-musl

build_configure() {
  if ! test -f ${configure_test:-Makefile}
  then
    ./${configure_cmd:-configure} --prefix="" \
      --host=aarch64-unknown-linux-gnu \
      --with-sysroot=${BUILD_DIR} \
      $configure_flags
  fi
  build_make
}

build_make() {
  if test -f ../config; then cp ../config ./.config; fi
  make -j 19 ${make_target}
  make ${install_target:-install}
}

build_meson() {
  if ! test -d _build
  then
    meson _build -Dprefix="/" --cross-file ${PROJROOT}/scripts/aarch64
  fi

  ninja -C _build
  ninja -C _build install
}

run_cmd() {
  test "$(type -t $1)" == "function" && $1
}

extract() {
  case $url in
    *.bz2)
      extract=j ;;
    *.gz)
      extract=z ;;
    *.xz)
      extract=J ;;
  esac
  mkdir -p src
  tar -x$extract -C src --strip-components 1 -f srcpkg
}

prepare_workspace() {
  cd $pkgpath

  if test -z "$url"
  then
    mkdir -p src
    return
  fi

  if test -n "$CLEAN"
  then
    rm -rf src
  fi

  if ! test -f srcpkg || ! test $(sha256sum srcpkg | head -c 64) == "$checksum"
  then
    rm -rf src
    curl -L $url -o srcpkg
  fi

  if ! test -d src
  then
    extract
  fi
}

do_build() {
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

  cd src
  run_cmd pre_build || true
  run_cmd build || run_cmd build_$type || true
  cd ..
}

copy_files() {
  if test -d files
  then
    cp -r files/* ${BUILD_DIR}/
    cp -r files/* ${INITRD}/
  fi

  for file in $files
  do
    cp -r ${BUILD_DIR}/$file ${INITRD}/$file
  done
}


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

copy_binaries() {
  for bin in $binaries
  do
    path=${BUILD_DIR}/$bin
    if test -e $path
    then
      process_elf $bin

      if ! test -L $path
      then
        cp $path ${INITRD}/bin/
      else
        ln -fs $(basename $(readlink $path)) ${INITRD}/bin/$(basename $bin)
        cp -L $(dirname $path)/$(readlink $path) ${INITRD}/bin/
      fi
    fi
  done
}

build_dependencies() {
  for dep in $depends
  do
    if ! ( test -f $BUILT_PACKAGES/$dep || $0 $dep )
    then
      echo "ERROR: $pkg requires $dep but it could not be automatically built"
      exit 1
    fi
  done
}

load_pkg() {
  pkgpath=pkgs/$pkg

  if test -f $pkgpath/build
  then
    . $pkgpath/build
  fi

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
}

mark_built() {
  for built in $provides $pkg
  do
    touch ${BUILT_PACKAGES}/$built
  done
}

if test "$1" == "--clean"
then
  export CLEAN=1
  shift
fi

pkg=$1

mkdir -p $BUILD_DIR $BUILT_PACKAGES ${INITRD}/{bin,lib} ${BOOTPART}

load_pkg
build_dependencies
prepare_workspace
do_build
copy_files
copy_binaries
mark_built
