/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Simple test that calls syslog with a long message.
 * Before the fix this could smash the stack in send_syslog_message.
 */

#include <stdio.h>
#include <string.h>
#include <syslog.h>

#define LONG_LEN 4096

int
main(void)
{
	char long_msg[LONG_LEN + 1];
	memset(long_msg, 'A', LONG_LEN);
	long_msg[LONG_LEN] = '\0';

	openlog("syslog-long-test", LOG_PID | LOG_CONS, LOG_USER);
	syslog(LOG_INFO, "short message");
	syslog(LOG_INFO, "long message: %s", long_msg);
	closelog();

	printf("syslog calls finished without crashing\n");
	return 0;
}
