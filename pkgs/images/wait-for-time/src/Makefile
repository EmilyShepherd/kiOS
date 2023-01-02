
wait-for-time: main.c wait_for_time.o
	$(CC) $(CFLAGS) $(LDFLAGS) -D SENSIBLE_TIME=$(SENSIBLE_TIME) -o $@ $^ 

wait_for_time.o: wait_for_time.c
	$(CC) $(CFLAGS) -D SENSIBLE_TIME=$(SENSIBLE_TIME) -c $^

.PHONY: install

install: wait-for-time wait_for_time.o
	cp wait-for-time $(DESTDIR)/bin/
	cp wait_for_time.o $(DESTDIR)/lib/
	cp wait_for_time.h $(DESTDIR)/include/
