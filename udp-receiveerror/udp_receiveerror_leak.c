/*
 * Reproducer for Haiku UDP ReceiveError leak
 * (ICMP error_received owns the buffer; early returns did not free it).
 *
 * Sends ICMP Destination Unreachable to 127.0.0.1 whose embedded
 * "original" packet is IPv4 + only 3 payload bytes. After ICMP/IPv4
 * strip the UDP module sees size < 4 and used to return without free.
 *
 * Build on Haiku:  gcc -O2 -o udp_receiveerror_leak udp_receiveerror_leak.c -lnetwork
 * Run:             ./udp_receiveerror_leak
 *                  ./udp_receiveerror_leak 127.0.0.1 20
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
#include <OS.h>
#endif

#ifndef ICMP_DEST_UNREACH
#define ICMP_DEST_UNREACH 3
#endif
#ifndef ICMP_PORT_UNREACH
#define ICMP_PORT_UNREACH 3
#endif

static uint16_t
checksum(const void *data, size_t len)
{
	const uint16_t *p = (const uint16_t *)data;
	uint32_t sum = 0;

	while (len > 1) {
		sum += *p++;
		len -= 2;
	}
	if (len == 1)
		sum += *(const uint8_t *)p;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += sum >> 16;
	return (uint16_t)~sum;
}

static void
print_memory(uint64_t sent)
{
#ifdef __HAIKU__
	system_info info;
	if (get_system_info(&info) == B_OK) {
		uint64_t used = (uint64_t)info.used_pages * B_PAGE_SIZE;
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
die(const char *what)
{
	perror(what);
	exit(1);
}

int
main(int argc, char **argv)
{
	const char *destName = "127.0.0.1";
	int seconds = 20;

	if (argc > 1)
		destName = argv[1];
	if (argc > 2)
		seconds = atoi(argv[2]);
	if (seconds < 1)
		seconds = 1;

	struct in_addr dest;
	if (inet_aton(destName, &dest) == 0) {
		fprintf(stderr, "bad address %s\n", destName);
		return 1;
	}

	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (fd < 0)
		die("socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)");

	/*
	 * ICMP header (8) + original IPv4 header (20) + 3 bytes.
	 * UDP ReceiveError bails out when leftover size < 4.
	 */
	unsigned char packet[8 + 20 + 3];
	memset(packet, 0, sizeof(packet));

	packet[0] = ICMP_DEST_UNREACH;
	packet[1] = ICMP_PORT_UNREACH;

	struct ip *inner = (struct ip *)(packet + 8);
	inner->ip_hl = 5;
	inner->ip_v = 4;
	inner->ip_len = htons(20 + 3);
	inner->ip_ttl = 64;
	inner->ip_p = IPPROTO_UDP;
	inner->ip_src = dest;
	inner->ip_dst = dest;
	inner->ip_sum = 0;
	inner->ip_sum = checksum(inner, 20);

	packet[28] = 0x12;
	packet[29] = 0x34;
	packet[30] = 0x56;

	*(uint16_t *)(packet + 2) = 0;
	*(uint16_t *)(packet + 2) = checksum(packet, sizeof(packet));

	struct sockaddr_in to;
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr = dest;

	printf("raw ICMP dest-unreach -> %s, inner UDP leftover=3 bytes\n",
		destName);
	printf("sending for %d seconds\n", seconds);
	printf("unpatched: used pages climb if ReceiveError drops without free\n");
	printf("patched:   used pages stay flat\n");
	printf("if socket() failed, this test needs raw ICMP (root on some OSes)\n");

	uint64_t sent = 0;
	time_t start = time(NULL);
	time_t lastPrint = start;

	while (time(NULL) - start < seconds) {
		if (sendto(fd, packet, sizeof(packet), 0,
				(struct sockaddr *)&to, sizeof(to)) < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK
				&& errno != EINTR && errno != ENOBUFS)
				perror("sendto");
		} else {
			sent++;
		}

		time_t now = time(NULL);
		if (now != lastPrint) {
			print_memory(sent);
			lastPrint = now;
		}
	}

	print_memory(sent);
	close(fd);
	return 0;
}
