/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * Reproducer for the UDP DeliverData leak: the clone is not freed when
 * Enqueue() returns ENOBUFS.
 *
 * Bind a UDP socket, join a multicast group, shrink SO_RCVBUF, and
 * never read. A second socket floods that group. After the FIFO is
 * full, each extra datagram clones a net_buffer that is leaked.
 *
 * Build on Haiku:
 *		make
 * Run:
 *		./udp_deliverdata_leak 60
 *
 * Watch the used-pages line. Unpatched: pages climb after the FIFO
 * fills. Patched: pages flatten. Ctrl-C stops. Link-local multicast
 * only.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __HAIKU__
#	include <OS.h>
#	include <SupportDefs.h>
#else
#	include <stdint.h>
	typedef uint16_t uint16;
	typedef uint64_t uint64;
#endif

static const char* kGroup = "239.255.42.1";
static const uint16 kPort = 42424;
static const int kReceiveBytes = 2048;
static const int kPayload = 512;

static void
PrintMemory(uint64 sent)
{
#ifdef __HAIKU__
	system_info info;
	if (get_system_info(&info) == B_OK) {
		uint64 used = (uint64)info.used_pages * B_PAGE_SIZE;
		printf("sent=%llu  system used=%llu bytes (%llu pages)\n",
			(unsigned long long)sent,
			(unsigned long long)used,
			(unsigned long long)info.used_pages);
		return;
	}
#endif
	printf("sent=%llu\n", (unsigned long long)sent);
}

static void
PrintErrorAndExit(const char* what)
{
	perror(what);
	exit(1);
}

int
main(int argc, char** argv)
{
	int seconds = 30;
	if (argc > 1)
		seconds = atoi(argv[1]);
	if (seconds < 1)
		seconds = 1;

	int receiveFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (receiveFd < 0)
		PrintErrorAndExit("socket recv");

	int yes = 1;
	setsockopt(receiveFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	int receiveBytes = kReceiveBytes;
	if (setsockopt(receiveFd, SOL_SOCKET, SO_RCVBUF, &receiveBytes,
			sizeof(receiveBytes)) < 0) {
		PrintErrorAndExit("SO_RCVBUF");
	}

	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(kPort);
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(receiveFd, (struct sockaddr*)&bindAddr, sizeof(bindAddr)) < 0)
		PrintErrorAndExit("bind");

	struct ip_mreq membership;
	memset(&membership, 0, sizeof(membership));
	membership.imr_multiaddr.s_addr = inet_addr(kGroup);
	membership.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(receiveFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership,
			sizeof(membership)) < 0) {
		PrintErrorAndExit("IP_ADD_MEMBERSHIP");
	}

	int sendFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendFd < 0)
		PrintErrorAndExit("socket send");

	unsigned char loop = 1;
	setsockopt(sendFd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	int ttl = 1;
	setsockopt(sendFd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	struct sockaddr_in destination;
	memset(&destination, 0, sizeof(destination));
	destination.sin_family = AF_INET;
	destination.sin_port = htons(kPort);
	destination.sin_addr.s_addr = inet_addr(kGroup);

	char payload[kPayload];
	memset(payload, 'U', sizeof(payload));

	printf("recv socket %d joined %s:%u, SO_RCVBUF=%d, not reading\n",
		receiveFd, kGroup, kPort, kReceiveBytes);
	printf("flooding for %d seconds (arg1 to change)\n", seconds);
	printf("unpatched: used pages climb after the FIFO fills\n");
	printf("patched:   used pages stay flat after the FIFO fills\n");

	uint64 sent = 0;
	time_t start = time(NULL);
	time_t lastPrint = start;

	while (time(NULL) - start < seconds) {
		if (sendto(sendFd, payload, sizeof(payload), 0,
				(struct sockaddr*)&destination, sizeof(destination)) < 0) {
			if (errno != ENOBUFS && errno != EAGAIN
				&& errno != EWOULDBLOCK && errno != EINTR)
				perror("sendto");
		} else
			sent++;

		time_t now = time(NULL);
		if (now != lastPrint) {
			PrintMemory(sent);
			lastPrint = now;
		}
	}

	PrintMemory(sent);
	close(sendFd);
	close(receiveFd);
	return 0;
}
