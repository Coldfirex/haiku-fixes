# virtio_block: ignore zero virtio blk_size

`virtio_block_set_capacity` does `capacity * 512 / blockSize`.
If the host sets `VIRTIO_BLK_F_BLK_SIZE` and `blk_size == 0`, that is
a divide-by-zero KDL. Also reachable from `virtio_block_config_callback`.

Treat zero as "use the 512-byte default".

No guest-only test: needs a virtio-blk device that advertises blk_size 0.
Normal QEMU virtio-blk uses 512 or 4096.
