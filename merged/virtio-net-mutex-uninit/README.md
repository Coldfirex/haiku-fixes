# virtio_net: destroy TX/RX mutexes if interrupt setup fails

Status: **merged** [Gerrit 11858](https://review.haiku-os.org/c/haiku/+/11858) / `1832af68`.
Do not re-push. Do not apply on current master.

- Change-Id: `I166bb2f012cd4fc8c787b227a0befc77f43808ba`
- Topic: `virtio-net`
- Code-Review: +2 waddlesplash
- Merged: 2026-09-29

We init rxLock and txLock, then call setup_interrupt. If that
fails we jumped to err6 and never destroyed the mutexes.

err6 is also used before those mutexes exist, so I added err7
and destroy them there.

`queue_setup_interrupt` still always returns B_OK on current Haiku.
The live fail path is PCI `setup_interrupt()`. Interrupt follow-up
stays in `hold/virtio-net-interrupt-uninit/`.
Do not amend 11784 or 11807. Drop the virtio_net overlay for this change.
