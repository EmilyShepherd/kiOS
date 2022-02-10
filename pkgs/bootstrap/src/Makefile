
LDFLAGS ?= -s

bootstrap: toml.c stretchy.c main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm bootstrap
