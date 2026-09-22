# virtio_gpu: delete framebuffer and shared areas in free()

Status: **merged**.

- Gerrit: https://review.haiku-os.org/c/haiku/+/11824
- Change-Id: `If1271be06991f36104ab285ba1eb0e1d3f05c6d0`
- Topic: `virtio-gpu`
- Merged: 2026-09-22

Do not re-push 11824. Do not apply this patch on current master.
Do not amend 11771 / 11776 / 11783.

After a successful `open()`, `close()` stopped the update thread and
deleted `commandDone`. `free()` waited for the thread, drained queues,
and freed the handle. Neither path deleted `framebufferArea` or
`sharedArea`. Drain, then `delete_area` both in `free()`.

After merge, drop any leftover overlay:

```
rm -f ~/config/non-packaged/add-ons/kernel/drivers/graphics/virtio_gpu
shutdown -r
```

Keep `virtio-gpu-open-shared-area` if that change is still local.
