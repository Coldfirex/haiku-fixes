# virtio_gpu: delete framebuffer and shared areas in free()

Status: **local**. Separate from `virtio-gpu-open-shared-area`.

After a successful `open()`, `close()` stops the update thread and
deletes `commandDone`. `free()` waits for the thread, drains queues,
and frees the handle. Neither path deletes `framebufferArea` or
`sharedArea`. `uninit_device()` only deletes `commandArea`.

Delete both areas in `free()` after `wait_for_thread()` so the update
thread is no longer touching the framebuffer.

Rebuild-only. Do not `open()` `/dev/graphics/...` from a tester on a
live desktop.

Overlay after jam + reboot:

`~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`

Apply independently of `virtio-gpu-open-shared-area` (different hunks).
Do not amend 11771 / 11776 / 11783. New Change-Id.

See `STATUS` and `virtio-gpu-free-areas.patch`.
