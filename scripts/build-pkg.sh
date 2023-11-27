#!/bin/bash
set -e

export PROJROOT=$(pwd)
BUILT_PACKAGES=$(pwd)/.build/built
BUILD_DIR=$(pwd)/.build/root
BOOTPART=$(pwd)/.build/bootpart
DATAPART=$(pwd)/.build/datapart

case ${HOST:-arm} in
  amd|x86)
    export ARCH=amd64
    export AARCH=x86_64
    ;;
  arm)
    export ARCH=arm64
    export AARCH=aarch64
esac

build_configure() {
  if test -n "$CLEAN" || ! test -f ${configure_test:-Makefile}
  then
    ./${configure_cmd:-configure} --prefix=$PREFIX \
      --host=${AARCH}-unknown-linux-musl \
      --build=x86_64-unknown-linux-gnu \
      --with-sysroot=${BUILD_DIR} \
      $configure_flags
  fi
  build_make
}

build_make() {
  test -n "$CLEAN" && make clean
  make -j 19 ${make_target}
  make ${install_target:-install}
}

build_meson() {
  if test -n "$CLEAN"
  then
    rm -rf _build
  fi

  if ! test -d _build
  then
    meson _build -Dprefix="/" --cross-file ${PROJROOT}/scripts/${AARCH} $meson_flags
  fi

  ninja -C _build ${make_target}
  ninja -C _build ${install_target:-install}
}

run_cmd() {
  test "$(type -t $1)" == "function" && $1
}

do_build() {
  export TARGET=${AARCH}-linux-musl
  export CGO_ENABLED=1
  export GOARCH=${ARCH}
  export CROSS_COMPILE=${TARGET}-
  export PREFIX="/"
  export DESTDIR=$BUILD_DIR
  export PKG_CONFIG_SYSTEM_LIBRARY_PATH=${BUILD_DIR}/lib
  export PKG_CONFIG_LIBDIR=${BUILD_DIR}/lib
  export PKG_CONFIG_PATH=${BUILD_DIR}/lib/pkgconfig
  export PKG_CONFIG_SYSROOT_DIR=${BUILD_DIR}
  export CC=$(which ${TARGET}-gcc)
  export CXX=$(which ${TARGET}-g++)
  export STRIP=$(which ${TARGET}-strip)

  local lto=""
  test -z "${nolto}" && lto="-flto"

  export CFLAGS="${lto} -isystem ${BUILD_DIR}/include -${optimise:-O2}"
  export LDFLAGS="${lto} -w -s -L${BUILD_DIR}/lib -Xlinker -rpath-link=${BUILD_DIR}/lib"

  cd src
  run_cmd pre_build || true
  run_cmd build || run_cmd build_$type || true
  run_cmd post_build || true
  cd ..
}

build_dependencies() {
  for dep in $depends
  do
    if ! ( test -f $BUILT_PACKAGES/${dep/\//_} || $0 $dep )
    then
      echo "ERROR: $pkg requires $dep but it could not be automatically built"
      exit 1
    fi
  done
}

load_pkg() {
  pkgpath=$(pwd)/pkgs/$pkg

  if test -f $pkgpath/build
  then
    . $pkgpath/build
  fi
}

mark_built() {
  for built in $provides $pkg
  do
    touch ${BUILT_PACKAGES}/${built/\//_}
  done
}

build_container() {
  if test -f $pkgpath/Containerfile
  then
    cd $BUILD_DIR
    image=docker.io/emilyls/$pkg:$version-$ARCH
    podman build \
      --no-cache --squash \
      --platform=linux/${ARCH} \
      -t $image \
      -f $pkgpath/Containerfile .
  fi
}

if test "$1" == "--clean"
then
  export CLEAN=1
  shift
fi

pkg=$1

mkdir -p $BUILD_DIR $BUILT_PACKAGES ${BOOTPART} ${DATAPART}

load_pkg
build_dependencies

cd $pkgpath
do_build
build_container
mark_built
