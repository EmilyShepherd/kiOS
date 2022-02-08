
init: init.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: install

install: init
	cp $^ $(DESTDIR)/bin/init-new
