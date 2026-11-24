# mouse-usb-device-not-ready

Regular mouse path in MouseInputDevice exits the control thread on any error other than B_INTERRUPTED.

USB HID checksum and transfer errors come back as B_DEVICE_NOT_READY. The mouse stays dead until unplug or reboot.

Patch: snooze 10 ms and try the read again.

Applies to RenkuOS and upstream Haiku. Proven on a Dell XPS with USB mouse.

No userspace test. Easiest repro is a USB mouse that occasionally drops packets or a flaky hub.
