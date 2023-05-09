
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
CMD := init

%.o: %.c
	$(CC) -flto -c $(CFLAGS) -o $@ $<

$(CMD): $(OBJS)
	$(CC) -flto $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: install
install: $(CMD)
	cp $^ $(DESTDIR)/init

.PHONY: clean
clean:
	rm -rf $(CMD) $(OBJS)
