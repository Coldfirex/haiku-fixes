/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * Reproducer for the UDP ReceiveError leak: ICMP error_received owns
 * the buffer; early returns did not free it.
 *
 * Sends ICMP Destination Unreachable to 127.0.0.1 whose embedded
 * "original" packet is IPv4 + only 3 payload bytes. After ICMP/IPv4
 * strip the UDP module sees size < 4 and used to return without free.
 *
 * Build on Haiku:
 *		make
 * Run:
 *		./udp_receiveerror_leak
 *		./udp_receiveerror_leak 127.0.0.1 20
 *
 * Default destination is loopback only. Pass another address only if
 * that address is this Haiku machine.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
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
	typedef uint8_t uint8;
	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint64_t uint64;
#endif

#ifndef ICMP_DEST_UNREACH
#	define ICMP_DEST_UNREACH 3
#endif
#ifndef ICMP_PORT_UNREACH
#	define ICMP_PORT_UNREACH 3
#endif

static uint16
InternetChecksum(const void* data, size_t length)
{
	const uint16* words = (const uint16*)data;
	uint32 sum = 0;

	while (length > 1) {
		sum += *words++;
		length -= 2;
	}
	if (length == 1)
		sum += *(const uint8*)words;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += sum >> 16;
	return (uint16)~sum;
}

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
	const char* destinationName = "127.0.0.1";
	int seconds = 20;

	if (argc > 1)
		destinationName = argv[1];
	if (argc > 2)
		seconds = atoi(argv[2]);
	if (seconds < 1)
		seconds = 1;

	struct in_addr destination;
	if (inet_aton(destinationName, &destination) == 0) {
		fprintf(stderr, "bad address %s\n", destinationName);
		return 1;
	}

	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (fd < 0)
		PrintErrorAndExit("socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)");

	/* ICMP header (8) + original IPv4 header (20) + 3 bytes.
	   UDP ReceiveError bails out when leftover size < 4. */
	unsigned char packet[8 + 20 + 3];
	memset(packet, 0, sizeof(packet));

	packet[0] = ICMP_DEST_UNREACH;
	packet[1] = ICMP_PORT_UNREACH;

	struct ip* inner = (struct ip*)(packet + 8);
	inner->ip_hl = 5;
	inner->ip_v = 4;
	inner->ip_len = htons(20 + 3);
	inner->ip_ttl = 64;
	inner->ip_p = IPPROTO_UDP;
	inner->ip_src = destination;
	inner->ip_dst = destination;
	inner->ip_sum = 0;
	inner->ip_sum = InternetChecksum(inner, 20);

	packet[28] = 0x12;
	packet[29] = 0x34;
	packet[30] = 0x56;

	*(uint16*)(packet + 2) = 0;
	*(uint16*)(packet + 2) = InternetChecksum(packet, sizeof(packet));

	struct sockaddr_in to;
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr = destination;

	printf("raw ICMP dest-unreach -> %s, inner UDP leftover=3 bytes\n",
		destinationName);
	printf("sending for %d seconds\n", seconds);
	printf("unpatched: used pages climb if ReceiveError drops without free\n");
	printf("patched:   used pages stay flat\n");
	printf("if socket() failed, this test needs raw ICMP\n");

	uint64 sent = 0;
	time_t start = time(NULL);
	time_t lastPrint = start;

	while (time(NULL) - start < seconds) {
		if (sendto(fd, packet, sizeof(packet), 0,
				(struct sockaddr*)&to, sizeof(to)) < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK
				&& errno != EINTR && errno != ENOBUFS)
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
	close(fd);
	return 0;
}
