/*
 * Copyright 2026 Alan Shearer. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Print the battery state, then wait.
 * Unplug the adapter and check syslog for acpi_battery.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <Drivers.h>

#include "device/power_managment.h"


static void
print_one(const char *path)
{
	int fd;
	acpi_battery_info info;
	status_t status;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("no %s\n", path);
		return;
	}

	status = ioctl(fd, GET_BATTERY_INFO, &info, sizeof(info));
	if (status != B_OK) {
		printf("%s GET_BATTERY_INFO failed: %s\n", path, strerror(status));
		close(fd);
		return;
	}

	printf("%s state 0x%lx rate %lu capacity %lu voltage %lu\n",
		path, (unsigned long)info.state, (unsigned long)info.current_rate,
		(unsigned long)info.capacity, (unsigned long)info.voltage);
	if ((info.state & BATTERY_DISCHARGING) != 0)
		printf("%s says: on battery\n", path);
	else
		printf("%s says: on AC\n", path);
	close(fd);
}


int
main(void)
{
	printf("waiting. unplug and replug the adapter.\n");
	for (;;) {
		print_one("/dev/power/acpi_battery/0");
		print_one("/dev/power/acpi_battery/1");
		sleep(1);
	}

	return 0;
}
