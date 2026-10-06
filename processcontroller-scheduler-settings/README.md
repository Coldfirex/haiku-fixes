ProcessController no longer owns the scheduler mode.

The Power saving menu item used to call set_scheduler_mode() and save
that in ProcessController's own settings. power_daemon is the only
thing that should apply the mode now.

The menu item launches application/x-vnd.Haiku-Power. Apply
power-preferences first, or the menu item has nothing to open.
