
wait-for-time: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -D SENSIBLE_TIME=$(SENSIBLE_TIME) -o $@ $^ 

.PHONY: install

install: wait-for-time
	cp $^ $(DESTDIR)/bin/
