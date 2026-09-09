/*
 * Reproducer for Haiku UDP DeliverData leak
 * (clone not freed when Enqueue returns ENOBUFS).
 *
 * Bind a UDP socket, join a multicast group, shrink SO_RCVBUF, and
 * never read. A second socket floods that group. After the FIFO is
 * full, each extra datagram clones a net_buffer that is leaked.
 *
 * Build on Haiku:  gcc -O2 -o udp_deliverdata_leak udp_deliverdata_leak.c -lnetwork
 * Run:             ./udp_deliverdata_leak
 * Watch:           used-pages line, or KDL "slabs" for net_buffer / data_node.
 *
 * Ctrl-C to stop. This talks to a link-local multicast group only.
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
#include <OS.h>
#endif

static const char *kGroup = "239.255.42.1";
static const uint16_t kPort = 42424;
static const int kRcvBytes = 2048;
static const int kPayload = 512;

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
	int seconds = 30;
	if (argc > 1)
		seconds = atoi(argv[1]);
	if (seconds < 1)
		seconds = 1;

	int recvFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (recvFd < 0)
		die("socket recv");

	int yes = 1;
	setsockopt(recvFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	int rcv = kRcvBytes;
	if (setsockopt(recvFd, SOL_SOCKET, SO_RCVBUF, &rcv, sizeof(rcv)) < 0)
		die("SO_RCVBUF");

	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(kPort);
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(recvFd, (struct sockaddr *)&bindAddr, sizeof(bindAddr)) < 0)
		die("bind");

	struct ip_mreq mreq;
	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(kGroup);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(recvFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
			sizeof(mreq)) < 0)
		die("IP_ADD_MEMBERSHIP");

	int sendFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendFd < 0)
		die("socket send");

	unsigned char loop = 1;
	setsockopt(sendFd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	int ttl = 1;
	setsockopt(sendFd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(kPort);
	dest.sin_addr.s_addr = inet_addr(kGroup);

	char payload[kPayload];
	memset(payload, 'U', sizeof(payload));

	printf("recv socket %d joined %s:%u, SO_RCVBUF=%d, not reading\n",
		recvFd, kGroup, kPort, kRcvBytes);
	printf("flooding for %d seconds (arg1 to change)\n", seconds);
	printf("unpatched: used pages climb after the FIFO fills\n");
	printf("patched:   used pages stay flat after the FIFO fills\n");

	uint64_t sent = 0;
	time_t start = time(NULL);
	time_t lastPrint = start;

	while (time(NULL) - start < seconds) {
		if (sendto(sendFd, payload, sizeof(payload), 0,
				(struct sockaddr *)&dest, sizeof(dest)) < 0) {
			if (errno != ENOBUFS && errno != EAGAIN
				&& errno != EWOULDBLOCK && errno != EINTR)
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
	close(sendFd);
	close(recvFd);
	return 0;
}
