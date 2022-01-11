
all: init build/bin/busybox modules

clean:
	rm -rf build

cleanall: clean
	rm -rf src

.PHONY: all clean cleanall init modules

# Init
build/etc/inittab build/etc/init.d/rcS build/etc/init.d/ip:
	mkdir -p build/etc
	cp -r files/* build/etc/

init: build/etc/inittab build/etc/init.d/rcS build/etc/init.d/ip
