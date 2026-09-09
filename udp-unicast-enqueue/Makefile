CC     ?= gcc
CFLAGS ?= -O2 -Wall
LIBS    = -lnetwork

udp_unicast_loopback: udp_unicast_loopback.c
	$(CC) $(CFLAGS) -o $@ udp_unicast_loopback.c $(LIBS)

clean:
	rm -f udp_unicast_loopback
