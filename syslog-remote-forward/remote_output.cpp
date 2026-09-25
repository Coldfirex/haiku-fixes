/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 */


#include "remote_output.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <driver_settings.h>


static const char* kMonths[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

// RFC 3164 traditional datagram size. Collectors accept this.
static const int kMaxDatagram = 1024;

static int sSocket = -1;
static bool sEnabled = false;
static sockaddr_storage sDest;
static socklen_t sDestLen = 0;
static char sHostname[256];


static void
refresh_hostname()
{
	if (gethostname(sHostname, sizeof(sHostname)) != 0
		|| sHostname[0] == '\0')
		strlcpy(sHostname, "haiku", sizeof(sHostname));
	sHostname[sizeof(sHostname) - 1] = '\0';
}


static int
syslog_pri(const syslog_message& message)
{
	int pri = message.priority;
	if (SYSLOG_FACILITY(pri) == 0)
		pri |= SYSLOG_FACILITY(message.options);
	return pri & 0xff;
}


static void
send_datagram(const char* buffer, int32 length)
{
	if (length <= 0 || sSocket < 0)
		return;

	sendto(sSocket, buffer, length, MSG_DONTWAIT,
		(const sockaddr*)&sDest, sDestLen);
	// ENETUNREACH / EAGAIN / EWOULDBLOCK: drop. Do not block the
	// logger-port thread on ARP or a missing route.
}


static void
remote_output(syslog_message& message)
{
	char header[160];
	char packet[kMaxDatagram];
	char stamp[32];
	struct tm when;
	int32 headerLength;
	int32 pos = 0;
	const char* ident;

	if (!sEnabled || sSocket < 0)
		return;

	localtime_r(&message.when, &when);
	if (when.tm_mon < 0 || when.tm_mon > 11)
		when.tm_mon = 0;
	snprintf(stamp, sizeof(stamp), "%s %2d %02d:%02d:%02d",
		kMonths[when.tm_mon], when.tm_mday, when.tm_hour, when.tm_min,
		when.tm_sec);

	ident = message.ident[0] != '\0' ? message.ident : "kernel";

	if ((message.options & LOG_PID) != 0) {
		headerLength = snprintf(header, sizeof(header),
			"<%d>%s %s %s[%