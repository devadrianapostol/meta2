.PHONY: all clean dist bootstrap snapshot check


CC = gcc
CFLAGS = -I. -g
META2 = bootstrap/meta2


all: meta2

clean:
	rm -f meta2.c meta2

meta2: meta2.c meta2.h
	$(CC) $(CFLAGS) $< -o $@

meta2.c: meta2.meta2
	$(META2) -q <$< >$@

snapshot: meta2.c
	cp $< bootstrap

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
	diff -u meta2.c meta2.2.c
