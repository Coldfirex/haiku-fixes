CC     ?= gcc
CFLAGS ?= -O2 -Wall
LIBS    = -lnetwork

udp_receiveerror_leak: udp_receiveerror_leak.c
	$(CC) $(CFLAGS) -o $@ udp_receiveerror_leak.c $(LIBS)

clean:
	rm -f udp_receiveerror_leak
