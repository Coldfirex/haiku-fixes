/*
 * Copyright 2026, Alan Shearer.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alan Shearer
 *
 * UDP sink for syslog-remote-forward. Listens for RFC 3164 datagrams
 * and exits 0 when COOKIE is seen. Timeout is a fail (daemon not
 * forwarding, or syslog_server not set).
 *
 * This is not a syslog server. It only proves the collector path.
 *
 * Build on Haiku (gcc 2.95 or gcc 13):
 *		make
 * Run (after the patched syslog_daemon is running with
 * syslog_server 127.0.0.1 and syslog_server_port 1514):
 *		./syslog_remote_sink 1514
 *		./syslog_remote_probe
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __HAIKU__
#	include <SupportDefs.h>
#else
#	include <stdint.h>
	typedef uint16_t uint16;
#endif


#define COOKIE "syslog-remote-forward-probe"


int
main(int argc, char** argv)
{
	int fd;
	int port;
	struct sockaddr_in local;
	struct timeval timeout;
	char buffer[2048];
	ssize_t bytes;

	port = 1514;
	if (argc > 1)
		port = atoi(argv[1]);
	if (port < 1 || port > 65535) {
		fprintf(stderr, "port out of range\n");
		return 1;
	}

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return 1;
	}

	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons((uint16)port);
	local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(fd, (struct sockaddr*)&local, sizeof(local)) < 0) {
		perror("bind");
		close(fd);
		return 1;
	}

	timeout.tv_sec = 8;
	timeout.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	printf("listening on 127.0.0.1:%d, waiting for %s\n", port, COOKIE);

	for (;;) {
		bytes = recvfrom(fd, buffer, sizeof(buffer) - 1, 0, NULL, NULL);
		if (bytes < 0) {
			if (errno == EINTR)
				continue;
			perror("recvfrom");
			close(fd);
			return 1;
		}
		buffer[bytes] = '\0';
		printf("%s\n", buffer);
		if (strstr(buffer, COOKIE) != NULL) {
			printf("cookie seen\n");
			close(fd);
			return 0;
		}
	}
}
