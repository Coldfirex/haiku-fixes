/*
 * Wait on /dev/power/acpi_video/0 and step brightness with screenmode.
 * 0x86 = up, 0x87 = down.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>


int
main(void)
{
	int fd = open("/dev/power/acpi_video/0", O_RDONLY);
	if (fd < 0) {
		perror("open /dev/power/acpi_video/0");
		return 1;
	}

	printf("waiting for brightness keys...\n");
	for (;;) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(fd, &set);
		if (select(fd + 1, &set, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			perror("select");
			break;
		}

		uint32 value = 0;
		size_t n = sizeof(value);
		if (read(fd, &value, &n) != (ssize_t)sizeof(value))
			continue;

		if (value == 0x86)
			system("screenmode -b +0.05");
		else if (value == 0x87)
			system("screenmode -b -0.05");
	}

	close(fd);
	return 0;
}
