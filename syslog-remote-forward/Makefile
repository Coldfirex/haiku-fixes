CC     ?= gcc
CFLAGS ?= -O2 -Wall
LIBS    = -lnetwork

all: syslog_remote_sink syslog_remote_probe

syslog_remote_sink: syslog_remote_sink.c
	$(CC) $(CFLAGS) -o $@ syslog_remote_sink.c $(LIBS)

syslog_remote_probe: syslog_remote_probe.c
	$(CC) $(CFLAGS) -o $@ syslog_remote_probe.c

clean:
	rm -f syslog_remote_sink syslog_remote_probe
