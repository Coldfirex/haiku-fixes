/*
 * Small userspace helper that watches the ACPI video device and
 * steps the brightness using the working accelerant path.
 *
 * Build with: gcc -o brightness-helper brightness-helper.c
 * Run as a background daemon or from UserBootScript.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <string.h>


#define DEVICE_PATH "/dev/power/acpi_video/0"
#define STEP 0.05f


static void
adjust_brightness(int up)
{
	char cmd[64];
	if (up)
		snprintf(cmd, sizeof(cmd), "screenmode -b +%.2f", STEP);
	else
		snprintf(cmd, sizeof(cmd), "screenmode -b -%.2f", STEP);
	system(cmd);
}


int
main(int argc, char** argv)
{
	int fd = open(DEVICE_PATH, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "cannot open %s: %s\n", DEVICE_PATH, strerror(errno));
		return 1;
	}

	printf("watching %s for brightness keys\n", DEVICE_PATH);

	while (1) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(fd, &set);

		int ret = select(fd + 1, &set, NULL, NULL, NULL);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror("select");
			break;
		}

		uint8 event = 0;
		ssize_t n = read(fd, &event, 1);
		if (n == 1) {
			if (event == 0x86) {
				printf("brightness up\n");
				adjust_brightness(1);
			} else if (event == 0x87) {
				printf("brightness down\n");
				adjust_brightness(0);
			}
		}
	}

	close(fd);
	return 0;
}
