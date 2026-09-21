# virtio_gpu: delete framebuffer and shared areas in free()

Status: **submitted** Gerrit 11824 (https://review.haiku-os.org/c/haiku/+/11824). Change-Id: If1271be06991f36104ab285ba1eb0e1d3f05c6d0. Separate from `virtio-gpu-open-shared-area`.

After a successful `open()`, `close()` stops the update thread and
deletes `commandDone`. `free()` waits for the thread, drains queues,
and frees the handle. Neither path deletes `framebufferArea` or
`sharedArea`. `uninit_device()` only deletes `commandArea`.

Delete both areas in `free()` after `wait_for_thread()` and
`virtio_gpu_drain_queues()` so the update thread is gone and
completed queue entries are reclaimed before backing memory is
released. This does not send DETACH_BACKING / RESOURCE_UNREF.

The driver stores the areas on the single device cookie. Accelerant
clones `sharedArea`; it does not get a second kernel `open()`. A
second `open()` of the node is still not refcounted.

Rebuild-only. Do not `open()` `/dev/graphics/...` from a tester on a
live desktop.

Overlay after jam + reboot:

`~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`

Apply independently of `virtio-gpu-open-shared-area` (different hunks).
Do not amend 11771 / 11776 / 11783. New Change-Id.

See `STATUS` and `virtio-gpu-free-areas.patch`.
