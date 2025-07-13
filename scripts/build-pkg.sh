#!/bin/bash
set -e

export PROJROOT=$(pwd)
BUILT_PACKAGES=$(pwd)/.build/built
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
  export PREFIX=$(pwd -P)

  if test -n "$CLEAN" || ! test -f ${configure_test:-Makefile}
  then
    if test -x autogen.sh
    then
      ./autogen.sh
    fi

    ./${configure_cmd:-configure} --prefix=$PREFIX \
      --host=${AARCH}-unknown-linux-musl \
      --build=x86_64-unknown-linux-gnu \
      --libdir='${prefix}/src/.libs' \
      --includedir="\${prefix}/${includedir:-include}" \
      $configure_flags
  fi
  build_make

  find -name '*.pc' | xargs -I{} cp {} ${PKG_CONFIG_PATH}/
}

build_make() {
  test -n "$CLEAN" && make clean
  make -j 19 ${make_target}
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
}

run_cmd() {
  test "$(type -t $1)" == "function" && $1
}

do_build() {
  export TARGET=${AARCH}-linux-musl
  export CGO_ENABLED=1
  export GOARCH=${ARCH}
  export CROSS_COMPILE=${TARGET}-
  export PKG_CONFIG_PATH=${PROJROOT}/pkgs/lib/pkgconfig
  export CC=$(which ${TARGET}-gcc)
  export CXX=$(which ${TARGET}-g++)
  export STRIP=$(which ${TARGET}-strip)

  local lto=""
  test -z "${nolto}" && lto="-flto"

  export CFLAGS="${lto} -${optimise:-O2}"
  export LDFLAGS="${lto} -w -s -L${PROJROOT}/pkgs/lib/musl/src/lib"

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

if test "$1" == "--clean"
then
  export CLEAN=1
  shift
fi

pkg=$1

mkdir -p $BUILT_PACKAGES ${BOOTPART} ${DATAPART}

load_pkg
build_dependencies

cd $pkgpath
do_build
mark_built
