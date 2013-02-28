.PHONY: all clean tar bootstrap snapshot


CC = gcc
CFLAGS = -I. -g
META2 = bootstrap/meta2
M2M2C = bootstrap/m2m2c


all: meta2 m2m2c

clean:
	rm -f meta2.c meta2

%.m2m: %.meta2
	$(META2) -c <$< >$@

%.c: %.m2m
	$(M2M2C) -q <$< >$@

meta2: meta2.c meta2.c
	$(CC) $(CFLAGS) $< -o $@

m2m2c: m2m2c.c meta2.h
	$(CC) $(CFLAGS) $< -o $@

snapshot: meta2.c m2m2c.c
	cp $^ bootstrap

tar:
	archive=meta2-$$(date +%Y%m%d); \
	rm -fr $$archive; \
	mkdir -p $$archive; \
	mkdir -p $$archive/bootstrap; \
	cp bootstrap/meta2.c $$archive/bootstrap; \
	cp bootstrap/m2m2c.c $$archive/bootstrap; \
	cp meta2.h meta2c Makefile README $$archive; \
	tar cfz $$archive.tgz $$archive; \
	rm -fr $$archive

bootstrap:
	cp bootstrap/meta2.c bootstrap/m2m2c.c .
	$(MAKE) all
	cp meta2 m2m2c bootstrap
