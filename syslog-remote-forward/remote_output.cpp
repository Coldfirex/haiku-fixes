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
			"<%d>%s %s %s[%ld]: ",
			syslog_pri(message), stamp, sHostname, ident,
			(long)message.from);
	} else {
		headerLength = snprintf(header, sizeof(header),
			"<%d>%s %s %s: ",
			syslog_pri(message), stamp, sHostname, ident);
	}
	if (headerLength < 0)
		return;
	if (headerLength >= (int32)sizeof(header))
		headerLength = sizeof(header) - 1;

	while (true) {
		const char* newLine = strchr(message.message + pos, '\n');
		int32 lineLength;
		int32 packetLength;
		int32 room;

		memcpy(packet, header, headerLength);
		room = (int32)sizeof(packet) - headerLength - 1;

		if (newLine != NULL)
			lineLength = newLine - (message.message + pos);
		else
			lineLength = strlen(message.message + pos);

		if (lineLength > room)
			lineLength = room;

		if (lineLength > 0)
			memcpy(packet + headerLength, message.message + pos, lineLength);

		packetLength = headerLength + lineLength;
		packet[packetLength] = '\0';

		if (lineLength > 0)
			send_datagram(packet, packetLength);

		if (newLine == NULL)
			break;

		pos += (newLine - (message.message + pos)) + 1;
		if (message.message[pos] == '\0')
			break;
	}
}


static bool
parse_destination(const char* server, uint16 port)
{
	sockaddr_in* in4;
	sockaddr_in6* in6;

	memset(&sDest, 0, sizeof(sDest));

	in4 = (sockaddr_in*)&sDest;
	if (inet_pton(AF_INET, server, &in4->sin_addr) == 1) {
		in4->sin_family = AF_INET;
		in4->sin_port = htons(port);
		sDestLen = sizeof(sockaddr_in);
		return true;
	}

	in6 = (sockaddr_in6*)&sDest;
	if (inet_pton(AF_INET6, server, &in6->sin6_addr) == 1) {
		in6->sin6_family = AF_INET6;
		in6->sin6_port = htons(port);
		sDestLen = sizeof(sockaddr_in6);
		return true;
	}

	return false;
}


void
init_remote_output(SyslogDaemon* daemon)
{
	void* handle;
	const char* server;
	const char* portParam;
	long portValue;
	int family;
	int flags;

	handle = load_driver_settings("kernel");
	if (handle == NULL)
		return;

	server = get_driver_parameter(handle, "syslog_server", NULL, NULL);
	portParam = get_driver_parameter(handle, "syslog_server_port", "514",
		"514");
	portValue = strtol(portParam != NULL ? portParam : "514", NULL, 0);
	if (portValue < 1 || portValue > 65535)
		portValue = 514;

	if (server == NULL || server[0] == '\0'
		|| !parse_destination(server, (uint16)portValue)) {
		unload_driver_settings(handle);
		return;
	}

	unload_driver_settings(handle);

	family = sDest.ss_family;
	sSocket = socket(family, SOCK_DGRAM, 0);
	if (sSocket < 0)
		return;

	flags = fcntl(sSocket, F_GETFL, 0);
	if (flags >= 0)
		fcntl(sSocket, F_SETFL, flags | O_NONBLOCK);

	refresh_hostname();
	sEnabled = true;
	daemon->AddHandler(remote_output);
}
