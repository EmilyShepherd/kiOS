
CFLAGS ?= -s -Os
OBJS := $(patsubst %.c,%.o,$(wildcard *.c */*.c))
CMD := init

CFLAGS += $(shell pkg-config --cflags zlib wolfssl) -I$(CURDIR)/include
LDFLAGS += $(shell pkg-config --libs zlib wolfssl) -L/home/emily/dev/kiOS/core/pkgs/lib/musl/src/lib

%.o: %.c
	$(CC) -flto -c $(CFLAGS) -o $@ $<

$(CMD): $(OBJS)
	$(CC) -flto $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: install
install: $(CMD)
	cp $^ $(DESTDIR)/init

.PHONY: clean
clean:
	rm -rf $(CMD) $(OBJS)
