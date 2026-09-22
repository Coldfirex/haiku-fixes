# virtio_block: ignore zero virtio blk_size (abandoned)

Gerrit [11833](https://review.haiku-os.org/c/haiku/+/11833) abandoned 2026-09-22.

`virtio_block_set_capacity` does `capacity * 512 / blockSize`.
A host that sets `VIRTIO_BLK_F_BLK_SIZE` and `blk_size == 0` would
divide by zero. That path is also in `virtio_block_config_callback`.

Virtio 1.1 §5.2.5 item 2 only says that if the feature is negotiated,
`blk_size` can be read for the optimal sector size; protocol units stay
512. It does not define a 512 fallback for 0.
https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.html#x1-2830005.2.5

QEMU writes `conf.logical_block_size` (512 or 4096). FreeBSD assigns
the field with no check. Linux fails probe on an invalid size.

Abandoned: unspecced workaround until a real emulator hits it.
Do not re-push. Do not apply on current master.
