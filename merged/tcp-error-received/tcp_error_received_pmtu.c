/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Hits TCPEndpoint::ErrorReceived() B_OK (ICMP fragmentation needed).
 * Not a flood. Do not raise the rate.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __HAIKU__
#	include <OS.h>
#endif

#ifndef ICMP_DEST_UNREACH
#	define ICMP_DEST_UNREACH 3
#endif
#ifndef ICMP_FRAG_NEEDED
#	define ICMP_FRAG_NEEDED 4
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
PrintMemory(unsigned long sent)
{
#ifdef __HAIKU__
	system_info info;
	if (get_system_info(&info) == B_OK) {
		printf("sent=%lu  system used=%llu bytes (%lu pages)\n",
			sent,
			(unsigned long long)info.used_pages * B_PAGE_SIZE,
			(unsigned long)info.used_pages);
		return;
	}
#endif
	printf("sent=%lu\n", sent);
}


int
main(int argc, char** argv)
{
	int seconds;
	int listenFd;
	int serverFd;
	int clientFd;
	int icmpFd;
	struct sockaddr_in addr;
	struct sockaddr_in clientAddr;
	struct sockaddr_in serverAddr;
	socklen_t addrLen;
	unsigned char packet[8 + 20 + 20];
	struct ip* inner;
	struct tcphdr* tcp;
	struct sockaddr_in to;
	unsigned long sent;
	int i;
	int ticks;

	seconds = 5;
	if (argc > 1)
		seconds = atoi(argv[1]);
	if (seconds < 1)
		seconds = 1;
	if (seconds > 10)
		seconds = 10;

	listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd < 0) {
		perror("socket listen");
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}
	addrLen = sizeof(addr);
	if (getsockname(listenFd, (struct sockaddr*)&addr, &addrLen) < 0) {
		perror("getsockname listen");
		return 1;
	}
	if (listen(listenFd, 1) < 0) {
		perror("listen");
		return 1;
	}

	clientFd = socket(AF_INET, SOCK_STREAM, 0);
	if (clientFd < 0) {
		perror("socket client");
		return 1;
	}
	if (connect(clientFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("connect");
		return 1;
	}

	addrLen = sizeof(serverAddr);
	serverFd = accept(listenFd, (struct sockaddr*)&serverAddr, &addrLen);
	if (serverFd < 0) {
		perror("accept");
		return 1;
	}

	addrLen = sizeof(clientAddr);
	if (getsockname(clientFd, (struct sockaddr*)&clientAddr, &addrLen) < 0) {
		perror("getsockname client");
		return 1;
	}

	icmpFd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmpFd < 0) {
		perror("socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)");
		return 1;
	}

	memset(packet, 0, sizeof(packet));
	packet[0] = ICMP_DEST_UNREACH;
	packet[1] = ICMP_FRAG_NEEDED;
	packet[6] = 0x02;
	packet[7] = 0x40;

	inner = (struct ip*)(packet + 8);
	inner->ip_hl = 5;
	inner->ip_v = 4;
	inner->ip_len = htons(40);
	inner->ip_ttl = 64;
	inner->ip_p = IPPROTO_TCP;
	inner->ip_src = clientAddr.sin_addr;
	inner->ip_dst = addr.sin_addr;
	inner->ip_sum = InternetChecksum(inner, 20);

	tcp = (struct tcphdr*)(packet + 28);
	tcp->th_sport = clientAddr.sin_port;
	tcp->th_dport = addr.sin_port;
	tcp->th_off = 5;

	*(uint16*)(packet + 2) = InternetChecksum(packet, sizeof(packet));

	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	printf("ICMP frag-needed -> 127.0.0.1, inner TCP %u -> %u\n",
		ntohs(clientAddr.sin_port), ntohs(addr.sin_port));
	printf("sending ~50/s for %d seconds (not a flood)\n", seconds);
	printf("expect: no KDL. used_pages may move a little either side.\n");

	sent = 0;
	ticks = seconds * 50;
	for (i = 0; i < ticks; i++) {
		if (sendto(icmpFd, packet, sizeof(packet), 0,
				(struct sockaddr*)&to, sizeof(to)) < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK
				&& errno != EINTR && errno != ENOBUFS)
				perror("sendto");
		} else
			sent++;
		if ((i % 50) == 49)
			PrintMemory(sent);
		usleep(20000);
	}

	PrintMemory(sent);
	close(icmpFd);
	close(clientFd);
	close(serverFd);
	close(listenFd);
	return 0;
}
