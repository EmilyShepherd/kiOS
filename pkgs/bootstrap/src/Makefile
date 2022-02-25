
LDFLAGS ?= -s

bootstrap: toml.c stretchy.c main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: clean install
clean:
	rm bootstrap

install: bootstrap
	cp bootstrap $(DESTDIR)/bin/
