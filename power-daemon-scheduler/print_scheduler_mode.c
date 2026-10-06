/*
 * Copyright 2026, Alan Shearer, sakison@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include <scheduler.h>
#include <stdio.h>


int
main(void)
{
	int32 mode = get_scheduler_mode();
	if (mode == SCHEDULER_MODE_POWER_SAVING)
		printf("power saving\n");
	else if (mode == SCHEDULER_MODE_LOW_LATENCY)
		printf("low latency\n");
	else
		printf("mode %ld\n", (long)mode);
	return 0;
}
