.PHONY: all clean


CC = gcc
CFLAGS = -I. -g
SNOBOL = snobol
SNOBOLFLAGS = 


all: meta2

clean:
	rm -f meta2.c meta2

meta2: meta2.c
	$(CC) $(CFLAGS) $< -o $@

meta2.c: m2m2c.sno meta2.m2m meta2.h
	$(SNOBOL) $(SNOBOLFLAGS) $< <meta2.m2m >$@
