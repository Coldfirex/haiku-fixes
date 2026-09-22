# virtio_block: delete DMAResource in uninit_driver

`dma_resource` is `new`'d in `virtio_block_init_driver`.
`uninit_device` deletes it. `init_device` can return before `*_cookie`
is set (`read_device_config` / `alloc_queues`), so `uninit_device` is
never called. `uninit_driver` must take the objects.

Overlay: `~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block`
Jam target is the virtio_block driver module. Reboot after overlay.

No userspace reproducer: failure path is `create_area`/`alloc_queues`/config
read under memory or host failure.
