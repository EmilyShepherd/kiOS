
init: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: install

install: init
	cp $^ $(DESTDIR)/bin/dhcp-cni-init
