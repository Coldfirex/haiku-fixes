CC     ?= gcc
CFLAGS ?= -O2 -Wall
LIBS    = -lnetwork

udp_deliverdata_leak: udp_deliverdata_leak.c
	$(CC) $(CFLAGS) -o $@ udp_deliverdata_leak.c $(LIBS)

clean:
	rm -f udp_deliverdata_leak
