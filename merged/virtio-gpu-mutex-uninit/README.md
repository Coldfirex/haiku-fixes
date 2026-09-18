# virtio_gpu: destroy commandLock if interrupt setup fails

Patch 2 of the virtio_gpu series. Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11776
- Change-Id: `I8eca66481988b3edb9b3e839e7feec0bfe86819c`
- haiku.git: `768d4e6cba315d55c9469c85d9219243408152d7`
- Topic: `virtio-gpu`
- Merged: 2026-09-17 (korli CR+2; waddlesplash submitted)
- Verified: Build OK rebasing over hrev60116

Do not re-push. Patch 1 is 11771. Patch 3 is 11783.

`mutex_init(&info->commandLock)` runs before `setup_interrupt()` /
`queue_setup_interrupt()`. Those failures jump to `err3`, which only
deleted the command area and left `commandLock` initialized. `err2` is
also used when `get_memory_map()` fails, before the mutex exists, so
destroy it only on `err3`.

Rebuild-only. No userspace flooder. Interrupt-setup failure is not
something you can force from Screen preferences. Smoke-test that the
driver still loads.
