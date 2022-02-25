
LDFLAGS ?= -s
LIB_PATH ?= $(PKG_CONFIG_SYSTEM_LIBRARY_PATH)

bootstrap: toml.c stretchy.c main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIB_PATH)/wait_for_time.o

.PHONY: clean install
clean:
	rm bootstrap

install: bootstrap
	cp bootstrap $(DESTDIR)/bin/
