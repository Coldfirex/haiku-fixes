/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * Userspace check for IPv4 source-filter copy-paste:
 * UnblockSource/DropSSM called Add() instead of Remove().
 *
 * Unpatched: a second IP_UNBLOCK_SOURCE or IP_DROP_SOURCE_MEMBERSHIP
 * still returns 0 because the source never left the set.
 * Patched: the second call returns EADDRNOTAVAIL.
 *
 * Build on Haiku:
 *		make
 * Run (needs IPv4, loopback is enough):
 *		./ipv4_multicast_filter
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


static int
Fail(const char* what)
{
	fprintf(stderr, "ipv4_multicast_filter: %s: %s\n", what, strerror(errno));
	return 1;
}


static int
OpenUdp()
{
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return -1;
	return fd;
}


static int
TestUnblock()
{
	int fd = OpenUdp();
	if (fd < 0)
		return Fail("socket");

	struct ip_mreq group;
	memset(&group, 0, sizeof(group));
	group.imr_multiaddr.s_addr = inet_addr("239.255.42.1");
	group.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group,
			sizeof(group)) != 0) {
		close(fd);
		return Fail("IP_ADD_MEMBERSHIP");
	}

	struct ip_mreq_source req;
	memset(&req, 0, sizeof(req));
	req.imr_multiaddr.s_addr = group.imr_multiaddr.s_addr;
	req.imr_interface.s_addr = htonl(INADDR_ANY);
	req.imr_sourceaddr.s_addr = inet_addr("192.0.2.1");

	if (setsockopt(fd, IPPROTO_IP, IP_BLOCK_SOURCE, &req, sizeof(req)) != 0) {
		close(fd);
		return Fail("IP_BLOCK_SOURCE");
	}
	if (setsockopt(fd, IPPROTO_IP, IP_UNBLOCK_SOURCE, &req, sizeof(req)) != 0) {
		close(fd);
		return Fail("IP_UNBLOCK_SOURCE");
	}

	int status = setsockopt(fd, IPPROTO_IP, IP_UNBLOCK_SOURCE, &req,
		sizeof(req));
	int saved = errno;
	setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &group, sizeof(group));
	close(fd);

	if (status == 0) {
		fprintf(stderr, "ipv4_multicast_filter: second UNBLOCK succeeded "
			"(source was not removed)\n");
		return 1;
	}
	if (saved != EADDRNOTAVAIL) {
		errno = saved;
		return Fail("second IP_UNBLOCK_SOURCE");
	}

	printf("unblock removed the source\n");
	return 0;
}


static int
TestDropSource()
{
	int fd = OpenUdp();
	if (fd < 0)
		return Fail("socket");

	struct ip_mreq_source req;
	memset(&req, 0, sizeof(req));
	req.imr_multiaddr.s_addr = inet_addr("239.255.42.2");
	req.imr_interface.s_addr = htonl(INADDR_ANY);
	req.imr_sourceaddr.s_addr = inet_addr("192.0.2.2");

	if (setsockopt(fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &req,
			sizeof(req)) != 0) {
		close(fd);
		return Fail("IP_ADD_SOURCE_MEMBERSHIP");
	}
	if (setsockopt(fd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, &req,
			sizeof(req)) != 0) {
		close(fd);
		return Fail("IP_DROP_SOURCE_MEMBERSHIP");
	}

	int status = setsockopt(fd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, &req,
		sizeof(req));
	int saved = errno;
	close(fd);

	if (status == 0) {
		fprintf(stderr, "ipv4_multicast_filter: second DROP_SOURCE succeeded "
			"(source was not removed)\n");
		return 1;
	}
	if (saved != EADDRNOTAVAIL) {
		errno = saved;
		return Fail("second IP_DROP_SOURCE_MEMBERSHIP");
	}

	printf("drop-source removed the source\n");
	return 0;
}


int
main()
{
	if (TestUnblock() != 0)
		return 1;
	if (TestDropSource() != 0)
		return 1;
	return 0;
}
