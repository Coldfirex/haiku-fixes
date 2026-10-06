/*
 * Copyright 2026 Alan Shearer. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Print the AC state, then wait.
 * Unplug the adapter and check syslog for power_daemon.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

#include <Drivers.h>

#include "device/power_managment.h"


static void
print_battery(void)
{
	int fd;
	acpi_battery_info info;
	status_t status;

	fd = open("/dev/power/acpi_battery/0", O_RDONLY);
	if (fd < 0) {
		printf("no /dev/power/acpi_battery/0\n");
		return;
	}

	status = ioctl(fd, GET_BATTERY_INFO, &info, sizeof(info));
	if (status != B_OK) {
		printf("GET_BATTERY_INFO failed: %s\n", strerror(status));
		close(fd);
		return;
	}

	printf("battery state 0x%lx rate %lu capacity %lu voltage %lu\n",
		(unsigned long)info.state, (unsigned long)info.current_rate,
		(unsigned long)info.capacity, (unsigned long)info.voltage);
	if ((info.state & BATTERY_DISCHARGING) != 0)
		printf("battery says: on battery\n");
	else
		printf("battery says: on AC\n");
	close(fd);
}


int
main(void)
{
	int fd;
	uint8 status;
	fd_set readSet;
	ssize_t n;

	print_battery();

	fd = open("/dev/power/acpi_ac/0", O_RDONLY);
	if (fd < 0) {
		printf("no /dev/power/acpi_ac/0. watch the battery line in syslog.\n");
		return 0;
	}

	n = read(fd, &status, 1);
	if (n == 1)
		printf("ac device status %u (%s)\n", status,
			status == 1 ? "online" : "offline");

	printf("waiting. unplug and replug the adapter.\n");
	for (;;) {
		FD_ZERO(&readSet);
		FD_SET(fd, &readSet);
		if (select(fd + 1, &readSet, NULL, NULL, NULL) < 0) {
			perror("select");
			break;
		}
		n = read(fd, &status, 1);
		if (n != 1)
			continue;
		printf("ac device status %u (%s)\n", status,
			status == 1 ? "online" : "offline");
	}

	close(fd);
	return 0;
}
