#!/bin/bash
set -e

export PROJROOT=$(pwd)
BUILT_PACKAGES=$(pwd)/.build/built
BOOTPART=$(pwd)/.build/bootpart
DATAPART=$(pwd)/.build/datapart

build_configure() {
  export PREFIX=$(pwd -P)

  if test -n "$CLEAN" || ! test -f ${configure_test:-Makefile}
  then
    if test -x autogen.sh
    then
      ./autogen.sh
    fi

    ./${configure_cmd:-configure} --prefix=$PREFIX \
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
    meson _build -Dprefix="/" $meson_flags
  fi

  ninja -C _build ${make_target}
}

run_cmd() {
  test "$(type -t $1)" == "function" && $1
}

do_build() {
  export CGO_ENABLED=1
  export PKG_CONFIG_PATH=${PROJROOT}/pkgs/lib/pkgconfig
  export LIBRARY_PATH=${PROJROOT}/pkgs/lib/musl/src/lib

  local lto=""
  test -z "${nolto}" && lto="-flto"

  export CFLAGS="${lto} -${optimise:-O2}"
  export LDFLAGS="${lto} -w -s"

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
