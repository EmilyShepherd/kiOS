
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
CMD := init

%.o: %.c
	$(CC) -flto -c $(CFLAGS) -o $@ $<

$(CMD): $(OBJS)
	$(CC) -flto -O3 $(LDFLAGS) -o $@ $^

.PHONY: install

install: init
	cp $^ $(DESTDIR)/init
