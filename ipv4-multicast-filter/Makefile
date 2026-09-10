CC     ?= gcc
CFLAGS ?= -O2 -Wall
LIBS    = -lnetwork

ipv4_multicast_filter: ipv4_multicast_filter.c
	$(CC) $(CFLAGS) -o $@ ipv4_multicast_filter.c $(LIBS)

clean:
	rm -f ipv4_multicast_filter
