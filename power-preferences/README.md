Power preferences

Two checkboxes. The window writes /boot/system/settings/power_daemon
and sends 'PwRl' to application/x-vnd.Haiku-powermanagement.

It does not call set_scheduler_mode(). power_daemon does that.

Apply this before power-daemon-scheduler. The app builds on its own.
The mode will not change until the daemon patch is in.
