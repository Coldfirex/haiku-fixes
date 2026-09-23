# virtio_block: delete DMAResource and IOScheduler in uninit_driver

`dma_resource` is `new`'d in `virtio_block_init_driver`.
`io_scheduler` is `new`'d in `virtio_block_set_capacity` (from
`init_device` after a successful config read).
`uninit_device` is the only deleter today.

`init_device` sets `*_cookie` only after `read_device_config` and
`alloc_queues`. Early returns skip `uninit_device`. Same for
`register_child_devices` failing after `init_driver` (`create_id` /
`publish_device`): teardown is `uninit_driver` only.
`alloc_queues` failing after `set_capacity` also leaves
`io_scheduler` allocated.

Delete both objects in `uninit_driver`. NULL the pointers in
`uninit_device` so the success unload path (both hooks) is not a
double-delete.

Not the 11833 `blk_size == 0` case. These are device-manager
error/teardown paths (publish fail, config read, queue alloc).

Overlay:
`~/config/non-packaged/add-ons/kernel/drivers/disk/virtual/virtio_block`
Jam the virtio_block driver module. Reboot after overlay.

No userspace reproducer.
