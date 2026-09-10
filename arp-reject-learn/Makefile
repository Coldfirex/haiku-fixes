CC     ?= gcc
CFLAGS ?= -O2 -Wall
LIBS    = -lnetwork

arp_reject_learn: arp_reject_learn.c
	$(CC) $(CFLAGS) -o $@ arp_reject_learn.c $(LIBS)

clean:
	rm -f arp_reject_learn
