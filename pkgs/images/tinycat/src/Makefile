
CFLAGS ?= -s -Os
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
CMD := tinycat

%.o: %.c
	$(CC) -flto -c $(CFLAGS) -o $@ $<

$(CMD): $(OBJS)
	$(CC) -flto $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: install
install:

.PHONY: clean
clean:
	rm -rf $(CMD) $(OBJS)
