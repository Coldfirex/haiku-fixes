power_daemon scheduler mode

Reads the settings file written by the Power preferences app.
Applies the mode when the daemon starts and when the AC adapter
changes. The preferences app can also send 'PwRl' to reload.

AC online is status == 1. That matches the old "ac status 1" printf.

Build the print helper on Haiku:

	cc print_scheduler_mode.c -o print_scheduler_mode

0 is low latency. 1 is power saving.
