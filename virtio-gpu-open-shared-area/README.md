# virtio_gpu: free shared info area if open() fails

Status: **local**. Kernel-only leftover after 11783.
Not `virtio-free-id` (that is virtio_net).

- 11771 detach-backing MERGED
- 11776 mutex-uninit MERGED
- 11783 clone-fd MERGED (accelerant)
- This change: `virtio_gpu_open()` shared-area leak

Do not amend 11771 / 11776 / 11783. New Change-Id.
Do not `open()` `/dev/graphics/...` from a tester on a live desktop.

Overlay after jam + reboot:

`~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu`

Not `drivers/bin`, not `accelerants/`.

`open()` creates the shared-info area then `goto error` without
`delete_area(sharedArea)`. `sharedArea` / `framebufferArea` were also
uninitialized, so an earlier failure called `delete_area()` on garbage.
Init both ids to `-1` and delete the shared area on the error path.

See `STATUS` and `virtio-gpu-open-shared-area.patch`.
