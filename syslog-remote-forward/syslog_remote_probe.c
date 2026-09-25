/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * Sends one userspace syslog() line with a unique cookie. The patched
 * daemon forwards it as RFC 3164 UDP if syslog_server is set. Stock
 * syslog_daemon writes the file only; syslog_remote_sink times out.
 *
 * Build on Haiku (gcc 2.95 or gcc 13):
 *		make
 * Run:
 *		./syslog_remote_probe
 */

#include <stdio.h>
#include <syslog.h>


#define COOKIE "syslog-remote-forward-probe"


int
main(void)
{
	openlog("syslog_remote_probe", LOG_PID, LOG_USER);
	syslog(LOG_INFO, "%s", COOKIE);
	closelog();
	printf("sent syslog %s\n", COOKIE);
	return 0;
}
