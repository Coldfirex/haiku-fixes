CC     ?= gcc
CFLAGS ?= -O2 -Wall
LIBS    = -lnetwork

tcp_error_received_pmtu: tcp_error_received_pmtu.c
	$(CC) $(CFLAGS) -o $@ tcp_error_received_pmtu.c $(LIBS)

clean:
	rm -f tcp_error_received_pmtu
