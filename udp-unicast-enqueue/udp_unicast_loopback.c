/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * Functional check for #18730 unicast Enqueue (no clone) plus loopback
 * TX checksum skip. Sends UDP datagrams to 127.0.0.1 and expects them
 * back. A checksum or ownership mistake drops or corrupts the payload.
 *
 * This is not an iperf3 stand-in. Measure throughput with:
 *		iperf3 -s
 *		iperf3 -c localhost -u -b 0 -t 20
 *
 * Build on Haiku:
 *		make
 * Run:
 *		./udp_unicast_loopback
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __HAIKU__
#	include <SupportDefs.h>
#else
#	include <stdint.h>
	typedef uint16_t uint16;
#endif

static const uint16 kPort = 42425;
static const int kPayload = 1472;
static const int kDatagrams = 256;

static void
PrintErrorAndExit(const char* what)
{
	perror(what);
	exit(1);
}

int
main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		PrintErrorAndExit("socket");

	struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(kPort);
	local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, (struct sockaddr*)&local, sizeof(local)) < 0)
		PrintErrorAndExit("bind");

	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	char payload[kPayload];
	memset(payload, 0x5a, sizeof(payload));

	int sent = 0;
	int received = 0;
	int mismatch = 0;

	for (int i = 0; i < kDatagrams; i++) {
		payload[0] = (char)i;
		if (sendto(fd, payload, sizeof(payload), 0,
				(struct sockaddr*)&local, sizeof(local)) < 0) {
			perror("sendto");
			continue;
		}
		sent++;

		char incoming[kPayload];
		ssize_t bytes = recvfrom(fd, incoming, sizeof(incoming), 0, NULL,
			NULL);
		if (bytes != (ssize_t)sizeof(payload)) {
			perror("recvfrom");
			continue;
		}
		if (memcmp(incoming, payload, sizeof(payload)) != 0)
			mismatch++;
		else
			received++;
	}

	close(fd);

	printf("sent=%d received=%d mismatch=%d\n", sent, received, mismatch);
	if (sent != kDatagrams || received != kDatagrams || mismatch != 0) {
		fprintf(stderr, "loopback unicast check failed\n");
		return 1;
	}
	printf("loopback unicast check ok\n");
	return 0;
}
