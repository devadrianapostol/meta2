PREFIX = /usr/local
CC = gcc
CFLAGS = -I. -g
META2 = bootstrap/meta2


.PHONY: all clean dist bootstrap snapshot check install rebuild


all: meta2

clean:
	rm -f meta2.c meta2

meta2: meta2.c meta2.h
	$(CC) $(CFLAGS) $< -o $@

meta2.c: meta2.meta2
	$(META2) -q <$< >$@

snapshot: all
	bash scripts/snapshot

rebuild:
	touch meta2.meta2
	$(MAKE) all

dist:
	archive=meta2-$$(date +%Y%m%d); \
	rm -fr $$archive; \
	mkdir -p $$archive; \
	mkdir -p $$archive/bootstrap; \
	cp bootstrap/meta2.c $$archive/bootstrap; \
	cp meta2.h meta2c Makefile README $$archive; \
	tar cfz $$archive.tgz $$archive; \
	rm -fr $$archive

bootstrap:
	cp bootstrap/meta2.c .
	$(MAKE) all
	cp meta2 bootstrap

check: all
	$(META2) -q <meta2.meta2 >meta2.c
	$(MAKE) meta2
	./meta2 -q <meta2.meta2 >meta2.2.c
	diff -bu meta2.c meta2.2.c
	./meta2 -cq <simple.meta2 >simple.c
	$(CC) $(CFLAGS) simple.c
	echo "3+4*(5+6.2e-5)" | ./a.out | cmp - simple.out

install: all
	mkdir -p $(PREFIX)/{bin,include}
	install -m755 $(META2) $(PREFIX)/bin
	install -m755 meta2c $(PREFIX)/bin
	install -m644 meta2.h $(PREFIX)/include
