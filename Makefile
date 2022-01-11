
BUSYBOX_VERSION=1.34.1

all: init build/bin/busybox modules

clean:
	rm -rf build

cleanall: clean
	rm -rf src

.PHONY: all clean cleanall init modules

# Busybox
#
BB_SRC=src/busybox-${BUSYBOX_VERSION}

${BB_SRC}/.config: busybox.config ${BB_SRC}/Makefile
	cp $< $@

${BB_SRC}/Makefile:
	mkdir -p src
	curl https://busybox.net/downloads/busybox-${BUSYBOX_VERSION}.tar.bz2 | tar -xjC src

${BB_SRC}/busybox: ${BB_SRC}/Makefile ${BB_SRC}/.config
	make -C src/busybox-1.34.1 ARCH=arm64 CROSS_COMPILE=aarch64-linux-musl-

build/bin/busybox: ${BB_SRC}/busybox
	mkdir -p build/bin
	cp $< $@

# Init
build/etc/inittab build/etc/init.d/rcS build/etc/init.d/ip:
	mkdir -p build/etc
	cp -r files/* build/etc/

init: build/etc/inittab build/etc/init.d/rcS build/etc/init.d/ip
